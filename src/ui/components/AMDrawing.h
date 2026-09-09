#pragma once

#include "ui/AntiMatrTheme.h"

namespace am::ui::draw
{

/** Soft glow around an ellipse: several concentric translucent rings. */
inline void glowEllipse (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, float radius, float intensity = 1.0f)
{
    if (radius <= 0.5f || intensity <= 0.001f) return;
    const int steps = juce::jlimit (3, 12, (int) (radius / 2.0f));
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float expand = radius * t;
        const float alpha = intensity * 0.10f * (1.0f - t) * (1.0f - t) + 0.004f;
        g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.fillEllipse (bounds.expanded (expand));
    }
}

/** Glow behind a rounded rectangle. */
inline void glowRoundedRect (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, juce::Colour colour, float radius, float intensity = 1.0f)
{
    if (radius <= 0.5f || intensity <= 0.001f) return;
    const int steps = juce::jlimit (3, 10, (int) (radius / 2.0f));
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float expand = radius * t;
        const float alpha = intensity * 0.08f * (1.0f - t) * (1.0f - t) + 0.003f;
        g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.fillRoundedRectangle (bounds.expanded (expand), corner + expand);
    }
}

/** Glowing stroked path (thin core line plus wide translucent halo). */
inline void glowPath (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float coreWidth, float haloWidth, float intensity = 1.0f)
{
    if (haloWidth > coreWidth)
    {
        const int steps = 4;
        for (int i = steps; i >= 1; --i)
        {
            const float t = (float) i / (float) steps;
            g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, intensity * 0.12f * (1.0f - t) + 0.02f)));
            g.strokePath (path, juce::PathStrokeType (coreWidth + (haloWidth - coreWidth) * t, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, 0.55f + 0.45f * intensity)));
    g.strokePath (path, juce::PathStrokeType (coreWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

/** Panel surface: vertical graphite gradient, subtle glass highlight and hairline border. */
inline void panelSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    juce::ColourGradient grad (Theme::panelTop, bounds.getX(), bounds.getY(), Theme::panel, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, corner);

    // top glass highlight
    juce::ColourGradient glass (Theme::glass, bounds.getX(), bounds.getY(), juce::Colours::transparentWhite, bounds.getX(), bounds.getY() + bounds.getHeight() * 0.35f, false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (Theme::border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

/** Inset (sunken) area inside a panel, e.g. behind a waveform. */
inline void insetSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (Theme::borderSoft);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

/** Thin arc used by knobs and rings. */
inline juce::Path arc (juce::Rectangle<float> bounds, float startRadians, float endRadians)
{
    juce::Path p;
    p.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f,
                     0.0f, startRadians, endRadians, true);
    return p;
}

/** Draws wide-tracked uppercase text. */
inline void trackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                         const juce::Font& font, juce::Colour colour)
{
    g.setFont (font);
    g.setColour (colour);
    g.drawText (text, area, just, false);
}

} // namespace am::ui::draw
