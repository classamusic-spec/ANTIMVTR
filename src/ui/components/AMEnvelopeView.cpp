#include "AMEnvelopeView.h"
#include "ui/UILayout.h"

namespace am::ui
{

AMEnvelopeView::AMEnvelopeView (juce::Colour c) : accent (c)
{
    setInterceptsMouseClicks (false, false);
}

void AMEnvelopeView::setEnvelope (float a, float d, float s, float r, float c)
{
    auto changed = [] (float x, float y) { return std::abs (x - y) > 1.0e-4f; };
    if (! (changed (a, attack) || changed (d, decay) || changed (s, sustain) || changed (r, release) || changed (c, curve)))
        return;
    attack = a; decay = d; sustain = juce::jlimit (0.0f, 1.0f, s); release = r; curve = juce::jlimit (-1.0f, 1.0f, c);
    repaint();
}

void AMEnvelopeView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (b.getWidth() < 8.0f || b.getHeight() < 8.0f) return;
    const float corner = juce::jlimit (4.0f, 8.0f, b.getHeight() * 0.1f);
    draw::insetSurface (g, b, corner);

    const float labelH = juce::jlimit (8.0f, 12.0f, b.getHeight() * 0.16f);
    auto plot = b.reduced (juce::jmax (5.0f, b.getWidth() * 0.02f), juce::jmax (4.0f, b.getHeight() * 0.09f))
                 .withTrimmedBottom (labelH);
    if (plot.getHeight() < 6.0f) return;

    // Horizontal rails at zero, sustain and full level.
    const auto stages = layout::envelopeStages (attack, decay, release);
    const float top = plot.getY(), bottom = plot.getBottom();
    auto levelY = [top, bottom] (float level) { return bottom - juce::jlimit (0.0f, 1.0f, level) * (bottom - top); };

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawLine (plot.getX(), top, plot.getRight(), top, 1.0f);
    g.drawLine (plot.getX(), bottom, plot.getRight(), bottom, 1.0f);
    g.setColour (accent.withAlpha (0.12f));
    g.drawLine (plot.getX(), levelY (sustain), plot.getRight(), levelY (sustain), 1.0f);

    // The curve control bends every segment the same way.
    auto shape = [this] (float t, bool rising)
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        const float bend = std::pow (2.0f, curve * 2.0f);        // 0.25 .. 4
        return rising ? std::pow (t, bend) : std::pow (t, 1.0f / bend);
    };

    const float w = plot.getWidth();
    const int steps = juce::jlimit (16, 160, (int) w);
    juce::Path path;
    path.startNewSubPath (plot.getX(), levelY (0.0f));

    auto segment = [&] (float fromX, float spanX, float fromLevel, float toLevel, bool rising)
    {
        if (spanX <= 0.5f) { path.lineTo (fromX + spanX, levelY (toLevel)); return; }
        const int n = juce::jmax (2, (int) (spanX / w * (float) steps));
        for (int i = 1; i <= n; ++i)
        {
            const float t = (float) i / (float) n;
            const float level = fromLevel + (toLevel - fromLevel) * shape (t, rising);
            path.lineTo (fromX + spanX * t, levelY (level));
        }
    };

    float x = plot.getX();
    segment (x, stages.attack * w, 0.0f, 1.0f, true);   x += stages.attack * w;
    segment (x, stages.decay * w, 1.0f, sustain, false); x += stages.decay * w;
    path.lineTo (x + stages.sustain * w, levelY (sustain)); x += stages.sustain * w;
    segment (x, stages.release * w, sustain, 0.0f, false);

    // Filled body plus a lit contour.
    {
        juce::Path body (path);
        body.lineTo (plot.getRight(), levelY (0.0f));
        body.lineTo (plot.getX(), levelY (0.0f));
        body.closeSubPath();
        juce::ColourGradient fill (accent.withAlpha (0.26f + 0.18f * activity), plot.getCentreX(), top,
                                   accent.withAlpha (0.02f), plot.getCentreX(), bottom, false);
        g.setGradientFill (fill);
        g.fillPath (body);
    }
    draw::glowPath (g, path, accent, juce::jmax (1.2f, plot.getHeight() * 0.022f), plot.getHeight() * 0.09f, 0.45f + 0.4f * activity);

    // Stage boundaries and their letters, so the shape is readable as A D S R.
    draw::screenGlass (g, b, corner, 0.9f);

    const juce::String names[] = { "A", "D", "S", "R" };
    const float spans[] = { stages.attack, stages.decay, stages.sustain, stages.release };
    auto labels = juce::Rectangle<float> (plot.getX(), plot.getBottom(), w, labelH);
    float cursor = plot.getX();
    for (int i = 0; i < 4; ++i)
    {
        const float span = spans[i] * w;
        if (i > 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawLine (cursor, top, cursor, bottom, 1.0f);
        }
        if (span > labelH * 0.9f)
            draw::trackedText (g, names[i], labels.withX (cursor).withWidth (span), juce::Justification::centred,
                               Theme::captionFont (labelH * 0.78f), Theme::textDim);
        cursor += span;
    }
}

} // namespace am::ui
