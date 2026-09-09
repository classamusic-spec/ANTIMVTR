#include "AMWaveView.h"

namespace am::ui
{

AMWaveView::AMWaveView()
{
    samples.assign (256, 0.0f);
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

void AMWaveView::mouseDown (const juce::MouseEvent& e)
{
    if (onArrow == nullptr) return;
    const int w = getWidth();
    if (e.x < w / 8) onArrow (-1);
    else if (e.x > w - w / 8) onArrow (1);
}

void AMWaveView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.1f);
    draw::insetSurface (g, b, corner);

    const float arrowW = juce::jmin (26.0f, b.getWidth() * 0.08f);
    auto inner = b.reduced (arrowW, b.getHeight() * 0.12f);
    const bool hasCaption = caption.isNotEmpty() && b.getHeight() > 60.0f;
    auto captionArea = hasCaption ? inner.removeFromBottom (juce::jmin (18.0f, b.getHeight() * 0.2f)) : juce::Rectangle<float>();

    // centre line
    g.setColour (Theme::borderSoft);
    g.drawLine (inner.getX(), inner.getCentreY(), inner.getRight(), inner.getCentreY(), 1.0f);

    // waveform
    juce::Path p;
    const int n = (int) samples.size();
    float peak = 0.0f;
    for (auto s : samples) peak = juce::jmax (peak, std::abs (s));
    const float norm = peak > 0.001f ? 0.9f / juce::jmax (peak, 0.25f) : 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float x = inner.getX() + inner.getWidth() * (float) i / (float) (n - 1);
        const float y = inner.getCentreY() - samples[(size_t) i] * norm * inner.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    draw::glowPath (g, p, accent, 1.2f, 7.0f, 0.5f + 0.5f * energy);

    // mirrored faint reflection for the "liquid" look
    {
        juce::Path mirror = p;
        mirror.applyTransform (juce::AffineTransform::verticalFlip (inner.getCentreY() * 2.0f));
        g.setColour (accent.withAlpha (0.12f));
        g.strokePath (mirror, juce::PathStrokeType (1.0f));
    }

    // arrows
    const float chev = juce::jmin (12.0f, arrowW * 0.5f);
    g.setColour (Theme::textSecondary);
    juce::Path left, right;
    left.startNewSubPath (b.getX() + arrowW * 0.65f, b.getCentreY() - chev * 0.5f);
    left.lineTo (b.getX() + arrowW * 0.35f, b.getCentreY());
    left.lineTo (b.getX() + arrowW * 0.65f, b.getCentreY() + chev * 0.5f);
    right.startNewSubPath (b.getRight() - arrowW * 0.65f, b.getCentreY() - chev * 0.5f);
    right.lineTo (b.getRight() - arrowW * 0.35f, b.getCentreY());
    right.lineTo (b.getRight() - arrowW * 0.65f, b.getCentreY() + chev * 0.5f);
    g.strokePath (left, juce::PathStrokeType (1.2f));
    g.strokePath (right, juce::PathStrokeType (1.2f));

    if (hasCaption)
    {
        const float h = juce::jlimit (8.0f, 10.5f, captionArea.getHeight() * 0.6f);
        draw::trackedText (g, juce::String::fromUTF8 ("\xE2\x80\xB9   ") + caption.toUpperCase() + juce::String::fromUTF8 ("   \xE2\x80\xBA"), captionArea, juce::Justification::centred, Theme::captionFont (h), Theme::textSecondary);
    }
}

//==============================================================================
void AMSpectrumView::setMagnitudes (const float* mags, int count)
{
    for (int i = 0; i < kBands; ++i)
    {
        const float src = count > 0 ? mags[juce::jmin (count - 1, i * count / kBands)] : 0.0f;
        bands[(size_t) i] = src;
        slow[(size_t) i] = juce::jmax (src, slow[(size_t) i] * 0.94f);
    }
    repaint();
}

void AMSpectrumView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.1f);
    draw::insetSurface (g, b, corner);

    auto inner = b.reduced (4.0f, 4.0f);
    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (inner.toNearestInt());

    // fragment grid
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    for (int i = 1; i < fragments; ++i)
    {
        const float x = inner.getX() + inner.getWidth() * (float) i / (float) fragments;
        g.drawLine (x, inner.getY(), x, inner.getBottom(), 1.0f);
    }

    auto buildPath = [&] (const std::array<float, kBands>& src, float scale, float shift)
    {
        juce::Path p;
        p.startNewSubPath (inner.getX(), inner.getBottom());
        for (int i = 0; i < kBands; ++i)
        {
            const float u = (float) i / (float) (kBands - 1);
            const float x = inner.getX() + inner.getWidth() * u;
            const float v = juce::jlimit (0.0f, 1.0f, src[(size_t) i] * scale + shift * std::sin (u * 9.0f));
            p.lineTo (x, inner.getBottom() - v * inner.getHeight() * 0.92f);
        }
        p.lineTo (inner.getRight(), inner.getBottom());
        p.closeSubPath();
        return p;
    };

    // layered translucent fills: blue (peak-hold), magenta (live)
    {
        auto p = buildPath (slow, 1.0f, 0.0f);
        juce::ColourGradient grad (Theme::blue.withAlpha (0.55f), 0, inner.getY(), Theme::blue.withAlpha (0.05f), 0, inner.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (p);
        g.setColour (Theme::cyan.withAlpha (0.6f));
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }
    {
        auto p = buildPath (bands, 0.85f, 0.0f);
        juce::ColourGradient grad (Theme::magenta.withAlpha (0.5f), 0, inner.getY(), Theme::violet.withAlpha (0.05f), 0, inner.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (p);
        g.setColour (Theme::magenta.withAlpha (0.7f));
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }

    // sparkle dots at band peaks when active
    if (activity > 0.05f)
    {
        g.setColour (Theme::textPrimary.withAlpha (0.35f * activity));
        for (int i = 0; i < kBands; i += 4)
        {
            const float u = (float) i / (float) (kBands - 1);
            const float x = inner.getX() + inner.getWidth() * u;
            const float y = inner.getBottom() - juce::jlimit (0.0f, 1.0f, slow[(size_t) i]) * inner.getHeight() * 0.92f;
            g.fillEllipse (x - 1.2f, y - 1.2f, 2.4f, 2.4f);
        }
    }
}

} // namespace am::ui
