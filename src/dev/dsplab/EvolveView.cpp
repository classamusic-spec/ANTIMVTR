#include "EvolveView.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    struct EvolveParam { Param param; const char* label; };

    const EvolveParam kEvolveParams[] =
    {
        { Param::evolveBend,    "Bend" },
        { Param::evolveMelt,    "Melt" },
        { Param::evolveTear,    "Tear" },
        { Param::evolveMagnet,  "Magnet" },
        { Param::evolveGravity, "Gravity" },
        { Param::evolveScatter, "Scatter" },
        { Param::evolveCrush,   "Crush" },
        { Param::evolveFreeze,  "Freeze" },
    };
}

EvolveView::EvolveView (MatterCaptureStore& c) : captures (c)
{
    beforePlot.setTitle ("Before capture");
    afterPlot.setTitle ("After capture");
    addAndMakeVisible (beforePlot);
    addAndMakeVisible (afterPlot);

    delta.setRowHeight (14.0f);
    delta.setLabelWidthFraction (0.62f);
    evolveParams.setRowHeight (14.0f);
    evolveParams.setLabelWidthFraction (0.5f);
    addAndMakeVisible (delta);
    addAndMakeVisible (evolveParams);

    styleButton (rawButton, Theme::magenta);
    styleButton (evolvedButton, Theme::violet);
    rawButton.onClick = [this] { applyBypass (true); };
    evolvedButton.onClick = [this] { applyBypass (false); };
    abPanel.addAndMakeVisible (rawButton);
    abPanel.addAndMakeVisible (evolvedButton);
    addAndMakeVisible (abPanel);

    for (const auto& p : kEvolveParams)
    {
        auto toggle = std::make_unique<juce::ToggleButton> (juce::String (p.label));
        styleToggle (*toggle, Theme::violet);
        toggle->setEnabled (false);
        toggle->setTooltip ("Per-operator bypass needs a per-operator flag in DevControls; the engine only exposes bypassEvolve today.");
        operatorPanel.addAndMakeVisible (*toggle);
        operatorToggles.push_back (std::move (toggle));
    }
    addAndMakeVisible (operatorPanel);
}

void EvolveView::applyBypass (bool raw)
{
    if (diagnostics != nullptr)
        diagnostics->dev.bypassEvolve.store (raw, std::memory_order_relaxed);
    rawButton.setToggleState (raw, juce::dontSendNotification);
    evolvedButton.setToggleState (! raw, juce::dontSendNotification);
}

void EvolveView::updateFrame (const LabFrame& f)
{
    diagnostics = &f.diagnostics;

    const bool raw = f.diagnostics.dev.bypassEvolve.load (std::memory_order_relaxed);
    rawButton.setToggleState (raw, juce::dontSendNotification);
    evolvedButton.setToggleState (! raw, juce::dontSendNotification);

    const auto& before = captures.before;
    const auto& after  = captures.after;

    beforePlot.setNodes (before.valid ? before.nodes.data() : nullptr, before.numNodes, before.fundamentalHz);
    beforePlot.setTitle (before.valid ? "Before capture" : "Before capture  (empty)");
    beforePlot.setSubtitle (before.valid ? before.describe()
                                         : juce::String ("CAPTURE BEFORE ON THE MATTER TAB"));

    // Without an AFTER capture the panel shows the live engine instead of
    // nothing, labelled so it is never mistaken for a capture.
    if (after.valid)
    {
        afterPlot.setNodes (after.nodes.data(), after.numNodes, after.fundamentalHz);
        afterPlot.setTitle ("After capture");
        afterPlot.setSubtitle (after.describe());
    }
    else
    {
        afterPlot.setNodes (f.snapshot.nodes, f.snapshot.numNodes, f.snapshot.fundamentalHz);
        afterPlot.setTitle ("After capture  (empty - showing LIVE)");
        afterPlot.setSubtitle ("LIVE ENGINE   " + juce::String (f.snapshot.numNodes) + " NODES");
    }

    if (before.valid && after.valid)
    {
        const auto d = compareDistributions (before.nodes.data(), before.numNodes,
                                             after.nodes.data(), after.numNodes);
        const auto statsBefore = analyseNodes (before.nodes.data(), before.numNodes, before.edges.data(), before.numEdges, before.fundamentalHz);
        const auto statsAfter  = analyseNodes (after.nodes.data(), after.numNodes, after.edges.data(), after.numEdges, after.fundamentalHz);

        delta.setRows ({
            { "Nodes before / after",    juce::String (d.nodesBefore) + " / " + juce::String (d.nodesAfter) },
            { "Clusters before / after", juce::String (d.clustersBefore) + " / " + juce::String (d.clustersAfter) },
            { "Nodes moved",             juce::String (d.movedNodes) + " / " + juce::String (juce::jmin (d.nodesBefore, d.nodesAfter)) },
            { "Mean movement",           juce::String (d.meanAbsCents, 2) + " cents" },
            { "Max movement",            juce::String (d.maxAbsCents, 2) + " cents" },
            { "Energy before / after",   juce::String (d.energyBefore, 5) + " / " + juce::String (d.energyAfter, 5) },
            { "Weight before / after",   juce::String (d.weightBefore, 3) + " / " + juce::String (d.weightAfter, 3) },
            { "HARMONICITY", "" },
            { "Mean cents before",       juce::String (statsBefore.meanAbsCents, 2) },
            { "Mean cents after",        juce::String (statsAfter.meanAbsCents, 2) },
            { "Edges before / after",    juce::String (statsBefore.numEdges) + " / " + juce::String (statsAfter.numEdges) },
            { "CPU", "" },
            { "Evolve moving %",         juce::String (f.snapshot.perf.movingPercent[(int) Subsystem::Evolve], 3) },
            { "Evolve peak %",           juce::String (f.snapshot.perf.peakPercent[(int) Subsystem::Evolve], 3) },
        });
        delta.clearRowColours();
        if (d.movedNodes > 0)
            delta.setRowColour (2, Theme::violet);
    }
    else
    {
        delta.setRows ({
            { "Status", before.valid || after.valid ? "one capture taken" : "no captures" },
            { "", "" },
            { "Use CAPTURE BEFORE / AFTER", "" },
            { "on the MATTER tab to compare", "" },
            { "a distribution across a change.", "" },
            { "CPU", "" },
            { "Evolve moving %", juce::String (f.snapshot.perf.movingPercent[(int) Subsystem::Evolve], 3) },
            { "Evolve peak %",   juce::String (f.snapshot.perf.peakPercent[(int) Subsystem::Evolve], 3) },
        });
        delta.clearRowColours();
    }

    // Effective Evolve parameter values straight from the control graph.
    const auto& control = f.processor.engine().control();
    std::vector<KeyValueTable::Row> rows;
    for (const auto& p : kEvolveParams)
    {
        const float base = control.baseValues()[(size_t) paramIndex (p.param)];
        const float eff  = control.values()[(size_t) paramIndex (p.param)];
        const float mod  = control.modulationOf (p.param);
        rows.push_back ({ juce::String (p.label),
                          juce::String (eff, 3) + (std::abs (mod) > 1.0e-4f
                              ? "  (" + juce::String (base, 3) + juce::String (mod >= 0.0f ? "+" : "") + juce::String (mod, 3) + ")"
                              : juce::String()) });
    }
    rows.push_back ({ "", "" });
    rows.push_back ({ "Engine bypassEvolve", raw ? "TRUE (raw)" : "false" });
    evolveParams.setRows (std::move (rows));
}

void EvolveView::resized()
{
    auto area = getLocalBounds();

    auto right = area.removeFromRight (juce::jmax (225, area.getWidth() * 28 / 100));
    area.removeFromRight (5);

    abPanel.setBounds (right.removeFromTop (50));
    {
        auto inner = abPanel.contentBounds();
        rawButton.setBounds (inner.removeFromLeft (inner.getWidth() / 2).reduced (2, 1));
        evolvedButton.setBounds (inner.reduced (2, 1));
    }
    right.removeFromTop (5);

    operatorPanel.setBounds (right.removeFromTop (juce::jmax (88, right.getHeight() * 24 / 100)));
    {
        auto inner = operatorPanel.contentBounds();
        const int columns = 2;
        const int rows = ((int) operatorToggles.size() + columns - 1) / columns;
        const int rowH = juce::jmax (16, inner.getHeight() / juce::jmax (1, rows));
        for (int r = 0; r < rows; ++r)
        {
            auto rowArea = inner.removeFromTop (rowH);
            for (int c = 0; c < columns; ++c)
            {
                const size_t index = (size_t) (r * columns + c);
                if (index >= operatorToggles.size()) break;
                operatorToggles[index]->setBounds (rowArea.removeFromLeft (rowArea.getWidth() / (columns - c)));
            }
        }
    }
    right.removeFromTop (5);

    delta.setBounds (right.removeFromTop (juce::jmax (170, right.getHeight() * 58 / 100)));
    right.removeFromTop (5);
    evolveParams.setBounds (right);

    beforePlot.setBounds (area.removeFromTop (area.getHeight() / 2));
    area.removeFromTop (5);
    afterPlot.setBounds (area);
}

} // namespace am::dev
