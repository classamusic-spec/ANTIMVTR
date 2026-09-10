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
    if (haloWidth > coreWidth && intensity > 0.01f)
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

/** Soft radial light (used for ambient glows behind displays and thumbnails). */
inline void softLight (juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour, float alpha)
{
    if (radius <= 1.0f || alpha <= 0.002f) return;
    juce::ColourGradient grad (colour.withAlpha (alpha), centre.x, centre.y, colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
}

/** A tiny glowing point of light. */
inline void glowDot (juce::Graphics& g, juce::Point<float> centre, float r, juce::Colour colour, float intensity = 1.0f)
{
    softLight (g, centre, r * 4.0f, colour, 0.35f * intensity);
    g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, 0.6f + 0.4f * intensity)));
    g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.55f * intensity));
    g.fillEllipse (centre.x - r * 0.45f, centre.y - r * 0.45f, r * 0.9f, r * 0.9f);
}

/** Panel surface: vertical graphite gradient, glass highlight, hairline border and a dark outer edge for depth. */
inline void panelSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    // Outer dark edge (separates the panel from the void without a hard shadow)
    g.setColour (Theme::panelEdge.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds.expanded (0.5f), corner + 0.5f, 1.0f);

    juce::ColourGradient grad (Theme::panelTop, bounds.getX(), bounds.getY(), Theme::panel, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, corner);

    // diagonal glass sheen in the top-left corner
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.035f), bounds.getX(), bounds.getY(),
                                juce::Colours::transparentWhite, bounds.getX() + bounds.getWidth() * 0.6f, bounds.getY() + bounds.getHeight() * 0.9f, false);
    g.setGradientFill (sheen);
    g.fillRoundedRectangle (bounds, corner);

    // top glass highlight line
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (bounds.getX() + corner, bounds.getY() + 1.0f, bounds.getRight() - corner, bounds.getY() + 1.0f, 1.0f);

    g.setColour (Theme::border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

/** Inset (sunken) area inside a panel, e.g. behind a waveform: darker fill with an inner shadow at the top. */
inline void insetSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (bounds, corner);
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.45f), bounds.getX(), bounds.getY(),
                                 juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + juce::jmin (18.0f, bounds.getHeight() * 0.3f), false);
    g.setGradientFill (shadow);
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (Theme::borderSoft);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

/** Sphere-like dark base used by knobs and selectors: radial shading, rim light, specular. */
inline void sphere (juce::Graphics& g, juce::Rectangle<float> circle, float lit = 0.0f)
{
    const auto c = circle.getCentre();
    const float r = circle.getWidth() * 0.5f;
    if (r < 1.0f) return;

    // body: light from the top-left
    juce::ColourGradient body (Theme::knobBase.brighter (0.4f + 0.25f * lit), c.x - r * 0.35f, c.y - r * 0.4f,
                               juce::Colour (0xff08080c), c.x + r * 0.6f, c.y + r * 0.7f, true);
    g.setGradientFill (body);
    g.fillEllipse (circle);

    // deep rim (bottom-right shadow)
    juce::ColourGradient rim (juce::Colours::transparentBlack, c.x, c.y, juce::Colours::black.withAlpha (0.55f), c.x, c.y + r, true);
    g.setGradientFill (rim);
    g.fillEllipse (circle);

    // rim light: thin bright arc on the upper-left edge
    {
        juce::Path arcPath;
        arcPath.addCentredArc (c.x, c.y, r - 0.8f, r - 0.8f, 0.0f, -juce::MathConstants<float>::pi * 0.95f, -juce::MathConstants<float>::pi * 0.05f, true);
        g.setColour (juce::Colours::white.withAlpha (0.16f + 0.10f * lit));
        g.strokePath (arcPath, juce::PathStrokeType (1.0f));
    }

    // specular
    softLight (g, { c.x - r * 0.3f, c.y - r * 0.38f }, r * 0.55f, juce::Colours::white, 0.07f + 0.05f * lit);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (circle, 1.0f);
}

/** Thin arc used by knobs and rings. */
inline juce::Path arc (juce::Rectangle<float> bounds, float startRadians, float endRadians)
{
    juce::Path p;
    p.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f,
                     0.0f, startRadians, endRadians, true);
    return p;
}

/** Small chevron (‹ ›) glyph: direction -1 = left, +1 = right. */
inline void chevron (juce::Graphics& g, juce::Rectangle<float> area, int direction, juce::Colour colour, float stroke = 1.2f)
{
    const float h = juce::jmin (area.getHeight(), area.getWidth() * 2.0f) * 0.5f;
    const float w = h * 0.5f;
    const auto c = area.getCentre();
    juce::Path p;
    if (direction < 0)
    {
        p.startNewSubPath (c.x + w * 0.5f, c.y - h * 0.5f); p.lineTo (c.x - w * 0.5f, c.y); p.lineTo (c.x + w * 0.5f, c.y + h * 0.5f);
    }
    else
    {
        p.startNewSubPath (c.x - w * 0.5f, c.y - h * 0.5f); p.lineTo (c.x + w * 0.5f, c.y); p.lineTo (c.x - w * 0.5f, c.y + h * 0.5f);
    }
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

/** 1 px hairline. */
inline void hairline (juce::Graphics& g, float x1, float y1, float x2, float y2, float alpha = 0.08f)
{
    g.setColour (juce::Colours::white.withAlpha (alpha));
    g.drawLine (x1, y1, x2, y2, 1.0f);
}

/**
    Draws tracked (letter-spaced) text. JUCE adds the extra kerning after the
    last glyph too, so centred / right-aligned text is nudged to compensate.
*/
inline void trackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                         const juce::Font& font, juce::Colour colour)
{
    const float trailing = font.getExtraKerningFactor() * font.getHeight();
    if (trailing > 0.0f)
    {
        if (just.testFlags (juce::Justification::horizontallyCentred)) area = area.translated (trailing * 0.5f, 0.0f);
        else if (just.testFlags (juce::Justification::right)) area = area.translated (trailing, 0.0f);
    }
    g.setFont (font);
    g.setColour (colour);
    g.drawText (text, area, just, false);
}

/** Width of tracked text (without the trailing kerning). */
inline float trackedTextWidth (const juce::Font& font, const juce::String& text)
{
    return juce::GlyphArrangement::getStringWidth (font, text) - font.getExtraKerningFactor() * font.getHeight();
}

/** Shrinks a font (down to minHeight) so the text fits the given width. */
inline juce::Font fitFont (const juce::Font& font, const juce::String& text, float maxWidth, float minHeight = 7.0f)
{
    const float w = juce::GlyphArrangement::getStringWidth (font, text);
    if (w <= maxWidth || w <= 0.0f) return font;
    const float scale = juce::jmax (minHeight / font.getHeight(), maxWidth / w);
    return scale < 0.999f ? font.withHeight (font.getHeight() * scale) : font;
}

} // namespace am::ui::draw
