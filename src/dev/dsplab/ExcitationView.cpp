#include "ExcitationView.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    const char* dryModeName (DryMode m) noexcept
    {
        switch (m)
        {
            case DryMode::FullSynth:       return "FULL PATH";
            case DryMode::SourceOnly:      return "SOURCE ONLY";
            case DryMode::MatterOnly:      return "MATTER ONLY";
            case DryMode::MatterAndEvolve: return "MATTER + EVOLVE";
            default:                       return "?";
        }
    }

    juce::Colour clusterColour (uint8_t cluster) noexcept
    {
        static const juce::Colour palette[] =
        {
            Theme::cyan, Theme::violet, Theme::amber, Theme::magenta,
            Theme::blue, Theme::ivory, Theme::cyan.brighter (0.3f), Theme::violet.darker (0.2f)
        };
        return palette[cluster % (uint8_t) (sizeof (palette) / sizeof (palette[0]))];
    }
}

//==============================================================================
ExcitationEnergyView::ExcitationEnergyView() : LabPanel ("Excitation energy per cluster") {}

void ExcitationEnergyView::setSnapshot (const DiagnosticSnapshot& s)
{
    clusters.clear();
    maxEnergy = 0.0f;
    numNodes = s.numNodes;

    for (int i = 0; i < s.numNodes; ++i)
    {
        const auto& n = s.nodes[i];
        auto it = std::find_if (clusters.begin(), clusters.end(),
                                [&] (const ClusterBar& c) { return c.cluster == n.cluster; });
        if (it == clusters.end())
        {
            clusters.push_back ({ n.cluster, 0.0f, 0.0f, 0 });
            it = clusters.end() - 1;
        }
        it->energy += std::isfinite (n.energy) ? n.energy : 0.0f;
        it->excitation += std::isfinite (n.excitation) ? n.excitation : 0.0f;
        ++it->nodes;
    }

    std::sort (clusters.begin(), clusters.end(),
               [] (const ClusterBar& a, const ClusterBar& b) { return a.cluster < b.cluster; });
    for (const auto& c : clusters)
        maxEnergy = juce::jmax (maxEnergy, c.energy);

    repaint();
}

void ExcitationEnergyView::paint (juce::Graphics& g)
{
    setSubtitle (juce::String (numNodes) + " NODES / " + juce::String ((int) clusters.size()) + " CLUSTERS");
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (clusters.empty())
    {
        plot::emptyState (g, area, "no nodes published");
        return;
    }

    const float rowH = juce::jmin (20.0f, area.getHeight() / (float) clusters.size());
    const float scale = maxEnergy > 1.0e-9f ? 1.0f / maxEnergy : 0.0f;

    for (const auto& c : clusters)
    {
        if (area.getHeight() < rowH) break;
        auto row = area.removeFromTop (rowH);
        auto label = row.removeFromLeft (30.0f);
        auto readout = row.removeFromRight (108.0f);
        auto bar = row.reduced (2.0f, rowH * 0.24f);

        const auto colour = clusterColour (c.cluster);
        plot::caption (g, "C" + juce::String ((int) c.cluster), label, colour, 8.5f);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (bar, 1.5f);
        const float w = juce::jlimit (0.0f, 1.0f, c.energy * scale) * bar.getWidth();
        g.setColour (colour.withAlpha (0.7f));
        g.fillRoundedRectangle (bar.withWidth (juce::jmax (1.0f, w)), 1.5f);

        g.setFont (Theme::font (9.5f));
        g.setColour (Theme::textSecondary);
        g.drawText (juce::String (c.energy, 5) + "  x" + juce::String (c.nodes), readout,
                    juce::Justification::centredRight, false);
    }
}

//==============================================================================
ExcitationView::ExcitationView()
{
    for (int i = 0; i < (int) DryMode::Count; ++i)
    {
        auto& b = dryButtons[i];
        b.setButtonText (dryModeName ((DryMode) i));
        b.setClickingTogglesState (false);
        styleButton (b, i == 0 ? Theme::cyan : Theme::amber);
        b.onClick = [this, i] { applyDryMode (i, diagnostics); };
        dryPanel.addAndMakeVisible (b);
    }
    addAndMakeVisible (dryPanel);

    inspector.setStage (Stage::Source);
    addAndMakeVisible (inspector);
    addAndMakeVisible (meters);
    addAndMakeVisible (energy);

    sourceInfo.setRowHeight (14.0f);
    addAndMakeVisible (sourceInfo);
}

void ExcitationView::applyDryMode (int mode, Diagnostics* diag)
{
    currentDryMode = juce::jlimit (0, (int) DryMode::Count - 1, mode);
    if (diag != nullptr)
        diag->dev.dryMode.store (currentDryMode, std::memory_order_relaxed);

    for (int i = 0; i < (int) DryMode::Count; ++i)
        dryButtons[i].setToggleState (i == currentDryMode, juce::dontSendNotification);
    repaint();
}

void ExcitationView::updateFrame (const LabFrame& f)
{
    diagnostics = &f.diagnostics;

    const int engineMode = f.diagnostics.dev.dryMode.load (std::memory_order_relaxed);
    if (engineMode != currentDryMode)
    {
        currentDryMode = juce::jlimit (0, (int) DryMode::Count - 1, engineMode);
        for (int i = 0; i < (int) DryMode::Count; ++i)
            dryButtons[i].setToggleState (i == currentDryMode, juce::dontSendNotification);
    }

    inspector.updateFrame (f);
    meters.setLevels (f.snapshot);
    energy.setSnapshot (f.snapshot);

    const auto& src = f.snapshot.stages[(int) Stage::Source];
    const auto& mtr = f.snapshot.stages[(int) Stage::PostMatter];
    const float gainDb = (src.rms > 1.0e-7f && mtr.rms > 1.0e-7f) ? gainToDb (mtr.rms / src.rms) : 0.0f;

    sourceInfo.setRows ({
        { "Dry mode (engine)", dryModeName ((DryMode) f.snapshot.dryMode) },
        { "Source rms / peak", dbString (src.rms) + " / " + dbString (src.peak) + " dB" },
        { "Matter rms / peak", dbString (mtr.rms) + " / " + dbString (mtr.peak) + " dB" },
        { "Matter gain",       juce::String (gainDb, 1) + " dB" },
        { "Active voices",     juce::String (f.snapshot.activeVoices) + " / " + juce::String (f.snapshot.maxVoices) },
        { "Focus note",        juce::String (f.snapshot.focusNote) + "  (" + juce::String (f.snapshot.fundamentalHz, 1) + " Hz)" },
        { "Matter energy",     juce::String (f.snapshot.matterEnergy, 5) },
        { "Nodes excited",     juce::String (f.snapshot.activeNodes) + " / " + juce::String (f.snapshot.numNodes) },
    });
}

void ExcitationView::resized()
{
    auto area = getLocalBounds();

    auto top = area.removeFromTop (50);
    dryPanel.setBounds (top);
    auto inner = dryPanel.contentBounds();
    const int bw = juce::jmax (60, inner.getWidth() / (int) DryMode::Count - 4);
    for (auto& b : dryButtons)
    {
        b.setBounds (inner.removeFromLeft (bw).reduced (1, 1));
        inner.removeFromLeft (4);
    }
    area.removeFromTop (5);

    auto right = area.removeFromRight (juce::jmax (210, area.getWidth() * 27 / 100));
    area.removeFromRight (5);

    meters.setBounds (right.removeFromTop (juce::jmax (118, right.getHeight() * 34 / 100)));
    right.removeFromTop (5);
    sourceInfo.setBounds (right.removeFromTop (juce::jmax (130, right.getHeight() * 52 / 100)));
    right.removeFromTop (5);
    energy.setBounds (right);

    inspector.setBounds (area);
}

} // namespace am::dev
