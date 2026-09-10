#include "ResonanceDistributionView.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    constexpr float kMinHz = 20.0f;
    constexpr float kMaxHz = 20000.0f;

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
FrequencyDistributionView::FrequencyDistributionView() : LabPanel ("Resonance distribution") {}

void FrequencyDistributionView::setNodes (const NodeDiag* n, int count, float f0)
{
    nodes = n;
    numNodes = juce::jmax (0, count);
    fundamentalHz = f0;
    repaint();
}

void FrequencyDistributionView::paint (juce::Graphics& g)
{
    setSubtitle (fundamentalHz > 0.0f ? "F0 " + juce::String (fundamentalHz, 2) + " HZ" : juce::String ("NO NOTE"));
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (nodes == nullptr || numNodes <= 0)
    {
        plot::emptyState (g, area, "no nodes published");
        return;
    }

    auto legend = area.removeFromTop (11.0f);
    plot::caption (g, "TARGET (DIM) VS ACTUAL   HEIGHT = ENERGY, GHOST = WEIGHT", legend, Theme::textDim, 8.5f);

    plot::frequencyGrid (g, area, kMinHz, kMaxHz, true);

    // Harmonic markers of the fundamental.
    if (fundamentalHz > 0.0f)
    {
        g.setColour (Theme::amber.withAlpha (0.16f));
        for (int h = 1; h <= 16; ++h)
        {
            const float hz = fundamentalHz * (float) h;
            if (hz > kMaxHz) break;
            const float x = area.getX() + logPosition (hz, kMinHz, kMaxHz) * area.getWidth();
            g.fillRect (x - 0.5f, area.getBottom() - 3.0f, 1.0f, 3.0f);
        }
    }

    float maxEnergy = 0.0f;
    for (int i = 0; i < numNodes; ++i)
        if (std::isfinite (nodes[i].energy))
            maxEnergy = juce::jmax (maxEnergy, nodes[i].energy);
    const float energyScale = maxEnergy > 1.0e-9f ? 1.0f / maxEnergy : 0.0f;

    const float plotHeight = area.getHeight() - 12.0f;

    for (int i = 0; i < numNodes; ++i)
    {
        const auto& n = nodes[i];
        if (! std::isfinite (n.frequency) || n.frequency <= 0.0f)
            continue;

        // Ghost bar: what the material asked for, scaled by weight.
        if (n.targetFrequency > 0.0f)
        {
            const float xt = area.getX() + logPosition (n.targetFrequency, kMinHz, kMaxHz) * area.getWidth();
            const float h = juce::jlimit (0.03f, 1.0f, std::isfinite (n.weight) ? n.weight : 0.0f) * plotHeight;
            g.setColour (Theme::textDim.withAlpha (0.55f));
            g.drawLine (xt, area.getBottom(), xt, area.getBottom() - h, 1.0f);
        }

        // Actual bar: where the node resonates now, height = energy.
        const float xa = area.getX() + logPosition (n.frequency, kMinHz, kMaxHz) * area.getWidth();
        const float energy = juce::jlimit (0.0f, 1.0f, (std::isfinite (n.energy) ? n.energy : 0.0f) * energyScale);
        const float h = juce::jmax (2.0f, energy * plotHeight);
        const auto colour = clusterColour (n.cluster);
        g.setColour (n.active != 0 ? colour.withAlpha (0.45f + 0.55f * energy) : colour.withAlpha (0.22f));
        g.drawLine (xa, area.getBottom(), xa, area.getBottom() - h, 1.6f);

        // Movement marker when Evolve has pulled a node away from its target.
        if (n.targetFrequency > 0.0f)
        {
            const float xt = area.getX() + logPosition (n.targetFrequency, kMinHz, kMaxHz) * area.getWidth();
            if (std::abs (xt - xa) > 1.5f)
            {
                g.setColour (Theme::magenta.withAlpha (0.30f));
                g.drawLine (xt, area.getBottom() - 2.0f, xa, area.getBottom() - 2.0f, 1.0f);
            }
        }
    }
}

//==============================================================================
RatioHistogramView::RatioHistogramView() : LabPanel ("Ratio histogram") {}

void RatioHistogramView::setNodes (const NodeDiag* nodes, int count, float fundamentalHz)
{
    numNodes = juce::jmax (0, count);
    ratioHistogram (nodes, numNodes, fundamentalHz, maxRatio, 64, bins);
    repaint();
}

void RatioHistogramView::paint (juce::Graphics& g)
{
    setSubtitle ("F / F0   0 - " + juce::String (maxRatio, 0));
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (bins.empty() || numNodes <= 0)
    {
        plot::emptyState (g, area, "no nodes published");
        return;
    }

    auto axis = area.removeFromBottom (10.0f);

    // Integer harmonic guides.
    for (int h = 1; h <= (int) maxRatio; ++h)
    {
        const float x = area.getX() + ((float) h / maxRatio) * area.getWidth();
        g.setColour (juce::Colours::white.withAlpha (h % 4 == 0 ? 0.09f : 0.04f));
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);
        if (h % 4 == 0)
            plot::caption (g, juce::String (h), juce::Rectangle<float> (x - 10.0f, axis.getY(), 20.0f, 10.0f),
                           Theme::textDim, 8.0f, juce::Justification::centred);
    }

    const float barW = area.getWidth() / (float) bins.size();
    for (size_t i = 0; i < bins.size(); ++i)
    {
        const float v = juce::jlimit (0.0f, 1.0f, bins[i]);
        if (v <= 0.0f) continue;
        const float x = area.getX() + (float) i * barW;
        const float h = juce::jmax (1.5f, v * area.getHeight());
        g.setColour (Theme::cyan.withAlpha (0.35f + 0.5f * v));
        g.fillRect (juce::Rectangle<float> (x, area.getBottom() - h, juce::jmax (1.0f, barW - 0.8f), h));
    }
}

//==============================================================================
HarmonicityView::HarmonicityView() : LabPanel ("Harmonicity deviation") {}

void HarmonicityView::setNodes (const NodeDiag* n, int count, float f0)
{
    nodes = n;
    numNodes = juce::jmax (0, count);
    fundamentalHz = f0;
    stats = analyseNodes (nodes, numNodes, nullptr, 0, fundamentalHz);
    repaint();
}

void HarmonicityView::paint (juce::Graphics& g)
{
    setSubtitle (numNodes > 0 && fundamentalHz > 0.0f
                 ? "MEAN " + juce::String (stats.meanAbsCents, 1) + " / MAX " + juce::String (stats.maxAbsCents, 1) + " CENTS"
                 : juce::String());
    LabPanel::paint (g);

    auto area = contentBoundsF();
    if (nodes == nullptr || numNodes <= 0 || fundamentalHz <= 0.0f)
    {
        plot::emptyState (g, area, "no note playing");
        return;
    }

    // Symmetric cents axis, at least +/- 25 cents so small deviations are visible.
    const float range = juce::jmax (25.0f, std::ceil (stats.maxAbsCents * 1.15f));

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (area.getX(), area.getCentreY(), area.getRight(), area.getCentreY(), 1.0f);
    for (float frac : { 0.5f, 1.0f })
        for (int sign : { -1, 1 })
        {
            const float y = area.getCentreY() - (float) sign * frac * area.getHeight() * 0.5f;
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
        }

    plot::caption (g, "+" + juce::String (range, 0) + "c", area.removeFromTop (10.0f), Theme::textDim, 8.0f);
    plot::caption (g, "-" + juce::String (range, 0) + "c",
                   juce::Rectangle<float> (area.getX(), area.getBottom() - 10.0f, 40.0f, 10.0f), Theme::textDim, 8.0f);

    plot::frequencyGrid (g, area, kMinHz, kMaxHz, false);

    for (int i = 0; i < numNodes; ++i)
    {
        const auto& n = nodes[i];
        if (! std::isfinite (n.frequency) || n.frequency <= 0.0f)
            continue;

        const float cents = centsFromNearestHarmonic (n.frequency, fundamentalHz);
        const float x = area.getX() + logPosition (n.frequency, kMinHz, kMaxHz) * area.getWidth();
        const float y = area.getCentreY() - juce::jlimit (-1.0f, 1.0f, cents / range) * area.getHeight() * 0.5f;
        const float radius = 1.4f + 2.6f * juce::jlimit (0.0f, 1.0f, std::isfinite (n.weight) ? n.weight : 0.0f);

        const auto colour = std::abs (cents) > 20.0f ? Theme::magenta : clusterColour (n.cluster);
        g.setColour (colour.withAlpha (n.active != 0 ? 0.9f : 0.35f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ x, y }));
        g.setColour (colour.withAlpha (0.18f));
        g.drawLine (x, area.getCentreY(), x, y, 1.0f);
    }
}

} // namespace am::dev
