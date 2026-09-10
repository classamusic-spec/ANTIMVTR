#include "AMWaveView.h"

namespace am::ui
{

AMWaveView::AMWaveView()
{
    samples.assign (256, 0.0f);
    trace.preallocateSpace (256 * 3 + 16);
    fill.preallocateSpace (256 * 3 + 16);
}

void AMWaveView::setSamples (const float* data, int numSamples)
{
    const int n = (int) samples.size();
    if (numSamples <= 0 || data == nullptr) { std::fill (samples.begin(), samples.end(), 0.0f); repaint(); return; }
    for (int i = 0; i < n; ++i)
    {
        const float pos = (float) i / (float) n * (float) (numSamples - 1);
        const int i0 = (int) pos;
        const int i1 = juce::jmin (numSamples - 1, i0 + 1);
        const float f = pos - (float) i0;
        samples[(size_t) i] = data[i0] + (data[i1] - data[i0]) * f;
    }
    repaint();
}

int AMWaveView::sideAt (juce::Point<int> p) const
{
    if (! arrows || onArrow == nullptr) return 0;
    const int w = getWidth();
    if (p.x < w / 8) return -1;
    if (p.x > w - w / 8) return 1;
    return 0;
}

void AMWaveView::mouseMove (const juce::MouseEvent& e) { const int s = sideAt (e.getPosition()); if (s != hoverSide) { hoverSide = s; repaint(); } }
void AMWaveView::mouseDown (const juce::MouseEvent& e) { const int s = sideAt (e.getPosition()); if (s != 0) onArrow (s); }

void AMWaveView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.1f);
    draw::insetSurface (g, b, corner);

    const float arrowW = arrows ? juce::jmin (26.0f, b.getWidth() * 0.08f) : 8.0f;
    auto inner = b.reduced (arrowW, b.getHeight() * 0.12f);
    const bool hasCaption = caption.isNotEmpty() && b.getHeight() > 60.0f;
    auto captionArea = hasCaption ? inner.removeFromBottom (juce::jmin (18.0f, b.getHeight() * 0.2f)) : juce::Rectangle<float>();

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (b.reduced (1.0f).toNearestInt());

        // grid: centre line + faint verticals
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawLine (inner.getX(), inner.getCentreY(), inner.getRight(), inner.getCentreY(), 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = inner.getX() + inner.getWidth() * (float) i / 8.0f;
            g.drawLine (x, inner.getY(), x, inner.getBottom(), 1.0f);
        }

        // ambient light behind the trace
        draw::softLight (g, inner.getCentre(), inner.getWidth() * 0.45f, accent, 0.05f + 0.08f * energy);

        // waveform
        const int n = (int) samples.size();
        float peak = 0.0f;
        for (auto s : samples) peak = juce::jmax (peak, std::abs (s));
        const float norm = peak > 0.001f ? 0.9f / juce::jmax (peak, 0.25f) : 0.0f;
        trace.clear();
        fill.clear();
        fill.startNewSubPath (inner.getX(), inner.getCentreY());
        for (int i = 0; i < n; ++i)
        {
            const float x = inner.getX() + inner.getWidth() * (float) i / (float) (n - 1);
            const float y = inner.getCentreY() - samples[(size_t) i] * norm * inner.getHeight() * 0.5f;
            if (i == 0) trace.startNewSubPath (x, y); else trace.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (inner.getRight(), inner.getCentreY());
        fill.closeSubPath();

        juce::ColourGradient grad (accent.withAlpha (0.16f + 0.1f * energy), 0.0f, inner.getY(), accent.withAlpha (0.02f), 0.0f, inner.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (fill);
        draw::glowPath (g, trace, accent, 1.3f, 8.0f, 0.45f + 0.55f * energy);

        // faint mirrored reflection for the liquid look
        {
            juce::Graphics::ScopedSaveState mirrorState (g);
            g.addTransform (juce::AffineTransform::verticalFlip (inner.getCentreY() * 2.0f));
            g.setColour (accent.withAlpha (0.10f));
            g.strokePath (trace, juce::PathStrokeType (1.0f));
        }
    }

    // arrows
    if (arrows)
    {
        const auto left  = juce::Rectangle<float> (b.getX(), b.getY(), arrowW, b.getHeight()).reduced (arrowW * 0.3f, b.getHeight() * 0.42f);
        const auto right = juce::Rectangle<float> (b.getRight() - arrowW, b.getY(), arrowW, b.getHeight()).reduced (arrowW * 0.3f, b.getHeight() * 0.42f);
        draw::chevron (g, left,  -1, hoverSide < 0 ? accent : Theme::textSecondary);
        draw::chevron (g, right,  1, hoverSide > 0 ? accent : Theme::textSecondary);
    }

    if (hasCaption)
    {
        const float h = juce::jlimit (7.5f, 10.0f, captionArea.getHeight() * 0.58f);
        const auto font = Theme::captionFont (h);
        const float w = draw::trackedTextWidth (font, caption);
        draw::trackedText (g, caption, captionArea, juce::Justification::centred, font, Theme::textSecondary);
        const auto cx = captionArea.getCentreX();
        draw::chevron (g, juce::Rectangle<float> (cx - w * 0.5f - 18.0f, captionArea.getY(), 10.0f, captionArea.getHeight()).reduced (0.0f, captionArea.getHeight() * 0.32f), -1, Theme::textDim, 1.0f);
        draw::chevron (g, juce::Rectangle<float> (cx + w * 0.5f + 8.0f, captionArea.getY(), 10.0f, captionArea.getHeight()).reduced (0.0f, captionArea.getHeight() * 0.32f), 1, Theme::textDim, 1.0f);
    }
}

//==============================================================================
AMSpectrumView::AMSpectrumView()
{
    live.preallocateSpace (kBands * 6 + 16);
    hold.preallocateSpace (kBands * 6 + 16);
    haze.preallocateSpace (kBands * 6 + 16);
}

void AMSpectrumView::setMagnitudes (const float* mags, int count)
{
    for (int i = 0; i < kBands; ++i)
    {
        const float src = count > 0 ? mags[juce::jmin (count - 1, i * count / kBands)] : 0.0f;
        bands[(size_t) i] = src;
        slow[(size_t) i]  = juce::jmax (src, slow[(size_t) i] * 0.93f);
        ghost[(size_t) i] = juce::jmax (src * 0.9f, ghost[(size_t) i] * 0.985f);
    }
    repaint();
}

void AMSpectrumView::buildMountain (juce::Path& p, const std::array<float, kBands>& src, juce::Rectangle<float> inner, float scale) const
{
    p.clear();
    p.startNewSubPath (inner.getX(), inner.getBottom() + 2.0f);
    auto pointAt = [&] (int i)
    {
        const float u = (float) i / (float) (kBands - 1);
        const float v = juce::jlimit (0.0f, 1.0f, src[(size_t) juce::jlimit (0, kBands - 1, i)] * scale);
        return juce::Point<float> (inner.getX() + inner.getWidth() * u, inner.getBottom() - v * inner.getHeight() * 0.9f);
    };
    // smooth ridge through midpoints for a mountain-like silhouette
    auto prev = pointAt (0);
    p.lineTo (prev);
    for (int i = 1; i < kBands; ++i)
    {
        const auto cur = pointAt (i);
        const auto mid = (prev + cur) * 0.5f;
        p.quadraticTo (prev, mid);
        prev = cur;
    }
    p.lineTo (prev);
    p.lineTo (inner.getRight(), inner.getBottom() + 2.0f);
    p.closeSubPath();
}

void AMSpectrumView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.1f);
    draw::insetSurface (g, b, corner);

    auto inner = b.reduced (6.0f, 6.0f);
    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (b.reduced (1.0f).toNearestInt());

    // fragment grid + horizontal hairlines
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (int i = 1; i < fragments; ++i)
    {
        const float x = inner.getX() + inner.getWidth() * (float) i / (float) fragments;
        g.drawLine (x, inner.getY(), x, inner.getBottom(), 1.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int i = 1; i < 4; ++i)
    {
        const float y = inner.getY() + inner.getHeight() * (float) i / 4.0f;
        g.drawLine (inner.getX(), y, inner.getRight(), y, 1.0f);
    }

    // layered translucent mountains: haze (violet, slowest), peak-hold (blue), live (magenta)
    buildMountain (haze, ghost, inner, 1.0f);
    {
        juce::ColourGradient grad (Theme::violet.withAlpha (0.22f), 0.0f, inner.getY(), Theme::violet.withAlpha (0.02f), 0.0f, inner.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (haze);
    }
    buildMountain (hold, slow, inner, 0.97f);
    {
        juce::ColourGradient grad (Theme::blue.withAlpha (0.42f), 0.0f, inner.getY(), Theme::blue.withAlpha (0.04f), 0.0f, inner.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (hold);
        g.setColour (Theme::cyan.withAlpha (0.55f));
        g.strokePath (hold, juce::PathStrokeType (1.0f));
    }
    buildMountain (live, bands, inner, 0.9f);
    {
        juce::ColourGradient grad (accent.withAlpha (0.48f), 0.0f, inner.getY(), Theme::violet.withAlpha (0.05f), 0.0f, inner.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (live);
        draw::glowPath (g, live, accent.withAlpha (0.85f), 1.0f, 5.0f, 0.25f + 0.6f * activity);
    }

    // baseline
    g.setColour (accent.withAlpha (0.35f));
    g.drawLine (inner.getX(), inner.getBottom(), inner.getRight(), inner.getBottom(), 1.0f);

    // sparkle dots at band peaks when active
    if (activity > 0.05f)
    {
        for (int i = 0; i < kBands; i += 4)
        {
            const float u = (float) i / (float) (kBands - 1);
            const float x = inner.getX() + inner.getWidth() * u;
            const float y = inner.getBottom() - juce::jlimit (0.0f, 1.0f, slow[(size_t) i] * 0.97f) * inner.getHeight() * 0.9f;
            draw::glowDot (g, { x, y }, 1.3f, Theme::textPrimary, 0.5f * activity);
        }
    }
}

} // namespace am::ui
