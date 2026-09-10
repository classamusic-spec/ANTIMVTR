#include "MatterInspector.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    LabTable::Cell numberCell (double value, int decimals, juce::Colour colour = Theme::textPrimary)
    {
        return { juce::String (value, decimals), value, colour };
    }

    LabTable::Cell intCell (int value, juce::Colour colour = Theme::textPrimary)
    {
        return { juce::String (value), (double) value, colour };
    }
}

MatterInspector::MatterInspector (MatterCaptureStore& c) : captures (c)
{
    sourceBox.addItem ("LIVE ENGINE", 1);
    sourceBox.addItem ("CAPTURE: BEFORE", 2);
    sourceBox.addItem ("CAPTURE: AFTER", 3);
    sourceBox.setSelectedId (1, juce::dontSendNotification);
    sourceBox.onChange = [this]
    {
        source = (Source) (sourceBox.getSelectedId() - 1);
        applySource();
    };
    styleCombo (sourceBox);
    addAndMakeVisible (sourceBox);

    styleToggle (clusterMode);
    clusterMode.onClick = [this] { topology.setClusterMode (clusterMode.getToggleState()); };
    addAndMakeVisible (clusterMode);

    for (auto* b : { &captureBefore, &captureAfter, &clearCaptures })
    {
        styleButton (*b);
        addAndMakeVisible (b);
    }
    captureBefore.onClick = [this]
    {
        if (haveSnapshot) captures.before.captureFrom (latest, "BEFORE");
        applySource();
    };
    captureAfter.onClick = [this]
    {
        if (haveSnapshot) captures.after.captureFrom (latest, "AFTER");
        applySource();
    };
    clearCaptures.onClick = [this]
    {
        captures.before.clear();
        captures.after.clear();
        source = Source::Live;
        sourceBox.setSelectedId (1, juce::dontSendNotification);
        applySource();
    };

    nodeTable.setColumns ({
        { "#",      30, true }, { "FREQ Hz",  66, true }, { "TARGET Hz", 66, true },
        { "RATIO",  52, true }, { "CENTS",    50, true }, { "ENERGY",    62, true },
        { "WEIGHT", 54, true }, { "DAMP",     58, true }, { "CLUSTER",   50, true },
        { "PAN",    46, true }, { "NL",       44, true }, { "EXC",       46, true },
        { "EDGES",  46, true }, { "ON",       32, true }
    });
    nodeTable.setRowColourFn ([this] (int row) -> juce::Colour
    {
        if (row < 0 || row >= latest.numNodes) return juce::Colours::transparentBlack;
        return latest.nodes[row].active != 0 ? juce::Colours::transparentBlack
                                             : juce::Colours::black.withAlpha (0.20f);
    });

    summary.setRowHeight (14.0f);
    summary.setLabelWidthFraction (0.58f);

    addAndMakeVisible (summary);
    addAndMakeVisible (nodePanel);
    nodePanel.addAndMakeVisible (nodeTable);
    addAndMakeVisible (distribution);
    addAndMakeVisible (topology);
    addAndMakeVisible (histogram);
    addAndMakeVisible (harmonicity);
}

void MatterInspector::applySource()
{
    const NodeDiag* nodes = nullptr;
    const EdgeDiag* edges = nullptr;
    int numNodes = 0, numEdges = 0;
    float f0 = 0.0f;
    juce::String label;

    if (source == Source::Live)
    {
        nodes = latest.nodes; edges = latest.edges;
        numNodes = latest.numNodes; numEdges = latest.numEdges;
        f0 = latest.fundamentalHz;
        label = "LIVE";
    }
    else
    {
        const auto& c = source == Source::Before ? captures.before : captures.after;
        if (c.valid)
        {
            nodes = c.nodes.data(); edges = c.edges.data();
            numNodes = c.numNodes; numEdges = c.numEdges;
            f0 = c.fundamentalHz;
        }
        label = c.valid ? c.describe() : juce::String ("capture empty");
    }

    distribution.setNodes (nodes, numNodes, f0);
    topology.setGraph (nodes, numNodes, edges, numEdges);
    histogram.setNodes (nodes, numNodes, f0);
    harmonicity.setNodes (nodes, numNodes, f0);
    nodePanel.setSubtitle (label);
}

void MatterInspector::updateFrame (const LabFrame& f)
{
    latest = f.snapshot;
    haveSnapshot = true;

    captureBefore.setButtonText (captures.before.valid ? "BEFORE *" : "CAPTURE BEFORE");
    captureAfter.setButtonText (captures.after.valid ? "AFTER *" : "CAPTURE AFTER");

    rebuildSummary (latest);
    if (source == Source::Live)
    {
        rebuildTable (latest);
        applySource();
    }
    else if (nodeTable.numRows() == 0)
    {
        rebuildTable (latest);
    }
}

void MatterInspector::rebuildTable (const DiagnosticSnapshot& s)
{
    const NodeDiag* nodes = s.nodes;
    int count = s.numNodes;
    float f0 = s.fundamentalHz;

    if (source != Source::Live)
    {
        const auto& c = source == Source::Before ? captures.before : captures.after;
        if (! c.valid) { nodeTable.setRows ({}); return; }
        nodes = c.nodes.data();
        count = c.numNodes;
        f0 = c.fundamentalHz;
    }

    std::vector<LabTable::Row> rows;
    rows.reserve ((size_t) count);

    for (int i = 0; i < count; ++i)
    {
        const auto& n = nodes[i];
        const float ratio = f0 > 0.0f && n.frequency > 0.0f ? n.frequency / f0 : 0.0f;
        const float cents = centsFromNearestHarmonic (n.frequency, f0);
        const auto centsColour = std::abs (cents) > 20.0f ? Theme::magenta : Theme::textPrimary;
        const auto energyColour = n.energy > 0.0f ? Theme::cyan : Theme::textDim;

        rows.push_back ({
            intCell (i, Theme::textSecondary),
            numberCell (n.frequency, 2),
            numberCell (n.targetFrequency, 2, Theme::textSecondary),
            numberCell (ratio, 3),
            numberCell (cents, 1, centsColour),
            numberCell (n.energy, 5, energyColour),
            numberCell (n.weight, 3),
            numberCell (n.damping, 5),
            intCell ((int) n.cluster),
            numberCell (n.pan, 2),
            numberCell (n.nonlinearity, 2),
            numberCell (n.excitation, 2),
            intCell ((int) n.couplingCount),
            { n.active != 0 ? "*" : "", n.active != 0 ? 1.0 : 0.0, n.active != 0 ? Theme::cyan : Theme::textDim }
        });
    }

    nodeTable.setRows (std::move (rows));
}

void MatterInspector::rebuildSummary (const DiagnosticSnapshot& s)
{
    const auto stats = analyseNodes (s.nodes, s.numNodes, s.edges, s.numEdges, s.fundamentalHz);

    summary.setRows ({
        { "Focus voice / note", juce::String (s.focusVoice) + " / " + juce::String (s.focusNote) },
        { "Fundamental",        juce::String (s.fundamentalHz, 2) + " Hz" },
        { "Nodes active/total", juce::String (s.activeNodes) + " / " + juce::String (s.numNodes) },
        { "Clusters",           juce::String (s.clusterCount) + "  (seen " + juce::String (stats.clusterCount) + ")" },
        { "Topology seed",      juce::String (s.topologySeed) },
        { "Matter energy",      juce::String (s.matterEnergy, 5) },
        { "Frequency span",     juce::String (stats.minFrequency, 1) + " - " + juce::String (stats.maxFrequency, 1) + " Hz" },
        { "Mean ratio",         juce::String (stats.meanRatio, 3) },
        { "Harmonicity",        juce::String (stats.meanAbsCents, 1) + " / " + juce::String (stats.maxAbsCents, 1) + " c" },
        { "COUPLING", "" },
        { "Edges",              juce::String (s.numEdges) },
        { "Average / max",      juce::String (s.averageCoupling, 3) + " / " + juce::String (s.maxCoupling, 3) },
        { "Mean per node",      juce::String (stats.meanCoupling, 2) },
        { "Edge strength",      juce::String (stats.meanStrength, 3) + " / " + juce::String (stats.maxStrength, 3) },
        { "MATERIAL", "" },
        { "A / B",              juce::String ((int) s.materialA) + " / " + juce::String ((int) s.materialB) },
        { "Blend",              juce::String (s.materialBlend, 3) },
        { "CAPTURES", "" },
        { "Before",             captures.before.valid ? captures.before.taken + "  " + juce::String (captures.before.numNodes) + "n" : "-" },
        { "After",              captures.after.valid ? captures.after.taken + "  " + juce::String (captures.after.numNodes) + "n" : "-" },
    });

    summary.clearRowColours();
    if (s.numNodes <= 1)
        summary.setRowColour (2, Theme::amber);
    if (s.numEdges == 0)
        summary.setRowColour (10, Theme::amber);
}

void MatterInspector::resized()
{
    auto area = getLocalBounds();

    auto controls = area.removeFromTop (24);
    sourceBox.setBounds (controls.removeFromLeft (150));
    controls.removeFromLeft (8);
    clusterMode.setBounds (controls.removeFromLeft (110));
    controls.removeFromLeft (8);
    captureBefore.setBounds (controls.removeFromLeft (120).reduced (0, 1));
    controls.removeFromLeft (4);
    captureAfter.setBounds (controls.removeFromLeft (120).reduced (0, 1));
    controls.removeFromLeft (4);
    clearCaptures.setBounds (controls.removeFromLeft (64).reduced (0, 1));
    area.removeFromTop (5);

    auto right = area.removeFromRight (juce::jmax (215, area.getWidth() * 28 / 100));
    area.removeFromRight (5);

    summary.setBounds (right.removeFromTop (juce::jmax (180, right.getHeight() * 46 / 100)));
    right.removeFromTop (5);
    topology.setBounds (right);

    distribution.setBounds (area.removeFromTop (juce::jmax (120, area.getHeight() * 34 / 100)));
    area.removeFromTop (5);

    auto lower = area.removeFromBottom (juce::jmax (94, area.getHeight() * 36 / 100));
    area.removeFromBottom (5);
    nodePanel.setBounds (area);
    nodeTable.setBounds (nodePanel.contentBounds());

    histogram.setBounds (lower.removeFromLeft (lower.getWidth() / 2));
    lower.removeFromLeft (5);
    harmonicity.setBounds (lower);
}

} // namespace am::dev
