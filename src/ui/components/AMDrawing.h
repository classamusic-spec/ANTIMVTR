#pragma once

#include "ui/AntiMatrTheme.h"

namespace am::ui::draw
{

/**
    The material language of the instrument.

    Everything the editor paints is assembled from the primitives below so the
    whole surface agrees about one thing: there is a single light, above and to
    the left. A raised slab is lit along its top-left edge and shadowed along
    its bottom-right; an inset well is exactly the opposite; a knob body is a
    dome with its specular in the upper-left third; every glow is the section
    accent at low opacity and never white.

    All of it derives from the bounds it is given, so the same code draws the
    same instrument at 1100 x 690 and at 1600 x 1000.
*/

/** Direction of the single light source, as a unit vector in component space. */
inline constexpr float kLightX = -0.5547f;
inline constexpr float kLightY = -0.8321f;

//==============================================================================
namespace detail
{
    /** Fine speckle for the chassis, built once. Tiled, so the cost is one fill. */
    inline const juce::Image& grainTile()
    {
        static const juce::Image tile = []
        {
            constexpr int n = 128;
            juce::Image img (juce::Image::ARGB, n, n, true);
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            juce::Random rng (0x51ede5);
            for (int y = 0; y < n; ++y)
                for (int x = 0; x < n; ++x)
                {
                    const float v = rng.nextFloat() * 2.0f - 1.0f;
                    // Slightly more dark speckle than light so large areas never lift.
                    const float a = std::abs (v) * (v < 0.0f ? 0.055f : 0.038f);
                    data.setPixelColour (x, y, (v < 0.0f ? juce::Colours::black : juce::Colours::white).withAlpha (a));
                }
            return img;
        }();
        return tile;
    }

    /**
        Horizontal brushed-metal streaks, built once.

        Each row is a sum of sine waves with whole-number frequencies across the
        tile, so the pattern is exactly periodic and the tile has no seam.
    */
    inline const juce::Image& brushTile()
    {
        static const juce::Image tile = []
        {
            constexpr int w = 256, h = 128;
            constexpr int harmonics = 4;
            juce::Image img (juce::Image::ARGB, w, h, true);
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            juce::Random rng (0xb1c5ed);
            for (int y = 0; y < h; ++y)
            {
                float freq[harmonics], phase[harmonics], amp[harmonics];
                float norm = 0.0f;
                for (int k = 0; k < harmonics; ++k)
                {
                    // Low frequencies along the row and a new draw for every row: long
                    // streaks in x, fine variation in y — which is what brushing looks like.
                    freq[k]  = (float) (1 + rng.nextInt (11));
                    phase[k] = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                    amp[k]   = 1.0f / (1.0f + (float) k);
                    norm += amp[k];
                }
                const float rowTone = (rng.nextFloat() * 2.0f - 1.0f) * 0.35f;
                for (int x = 0; x < w; ++x)
                {
                    const float u = (float) x / (float) w * juce::MathConstants<float>::twoPi;
                    float v = 0.0f;
                    for (int k = 0; k < harmonics; ++k) v += amp[k] * std::sin (u * freq[k] + phase[k]);
                    v = juce::jlimit (-1.0f, 1.0f, v / norm + rowTone);
                    data.setPixelColour (x, y, (v > 0.0f ? juce::Colours::white : juce::Colours::black).withAlpha (std::abs (v) * 0.085f));
                }
            }
            return img;
        }();
        return tile;
    }
}

//==============================================================================
// Textures
//==============================================================================

/** Fine chassis grain, so large flat areas never band. */
inline void grain (juce::Graphics& g, juce::Rectangle<float> bounds, float strength = 1.0f)
{
    if (strength <= 0.01f || bounds.isEmpty()) return;
    g.setTiledImageFill (detail::grainTile(), 0, 0, juce::jlimit (0.0f, 1.0f, strength));
    g.fillRect (bounds);
}

/** Barely visible brushed-metal streaks running horizontally, clipped to a rounded rectangle. */
inline void brushedStreaks (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float strength = 1.0f)
{
    if (strength <= 0.01f || bounds.getWidth() < 3.0f || bounds.getHeight() < 3.0f) return;
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip;
    clip.addRoundedRectangle (bounds, corner);
    g.reduceClipRegion (clip);
    g.setTiledImageFill (detail::brushTile(), juce::roundToInt (bounds.getX()), juce::roundToInt (bounds.getY()),
                         juce::jlimit (0.0f, 1.0f, strength));
    g.fillRect (bounds);
}

//==============================================================================
// Light and shadow
//==============================================================================

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

/** Contact shadow beneath a rounded shape: soft, offset down and to the right. */
inline void contactShadow (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float radius, float strength = 1.0f)
{
    if (radius < 0.5f || strength <= 0.01f) return;
    const int steps = juce::jlimit (3, 9, (int) (radius * 0.8f));
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float spread = radius * t;
        g.setColour (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, 0.20f * strength * (1.0f - t) * (1.0f - t) + 0.02f * strength)));
        g.fillRoundedRectangle (bounds.expanded (spread * 0.7f).translated (spread * 0.26f, spread * 0.52f), corner + spread * 0.7f);
    }
}

/** Contact shadow beneath a circular control. */
inline void contactShadowEllipse (juce::Graphics& g, juce::Rectangle<float> circle, float radius, float strength = 1.0f)
{
    if (radius < 0.5f || strength <= 0.01f) return;
    const int steps = juce::jlimit (3, 9, (int) (radius * 0.8f));
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float spread = radius * t;
        g.setColour (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, 0.22f * strength * (1.0f - t) * (1.0f - t) + 0.02f * strength)));
        g.fillEllipse (circle.expanded (spread * 0.55f).translated (spread * 0.28f, spread * 0.55f));
    }
}

/**
    Bevelled edge: a light line along the top-left of the shape and a dark one
    along the bottom-right, drawn as one diagonal gradient stroke so the two
    always agree about where the light is.
*/
inline void bevelEdge (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float light, float dark, float thickness = 1.2f)
{
    if (bounds.getWidth() < 2.0f || bounds.getHeight() < 2.0f) return;
    juce::Path p;
    p.addRoundedRectangle (bounds.reduced (thickness * 0.5f), juce::jmax (0.0f, corner - thickness * 0.5f));
    juce::ColourGradient grad (juce::Colours::white.withAlpha (light), bounds.getX(), bounds.getY(),
                               juce::Colours::black.withAlpha (dark), bounds.getRight(), bounds.getBottom(), false);
    grad.addColour (0.42, juce::Colours::white.withAlpha (light * 0.10f));
    grad.addColour (0.58, juce::Colours::black.withAlpha (dark * 0.10f));
    g.setGradientFill (grad);
    g.strokePath (p, juce::PathStrokeType (thickness));
}

//==============================================================================
// Surfaces
//==============================================================================

/** How a raised slab is finished. The defaults give the standard instrument panel. */
struct SlabStyle
{
    juce::Colour top    = Theme::panelTop;
    juce::Colour bottom = Theme::panel;
    float shadow = 1.0f;   ///< drop shadow beneath the slab
    float bevel  = 1.0f;   ///< light top-left / dark bottom-right edge
    float brush  = 1.0f;   ///< brushed-metal streaks
    float sheen  = 1.0f;   ///< broad diagonal light from the top-left
};

/**
    A raised slab: the panel material. Vertical gradient from a lit top to a
    dark bottom, brushed horizontally, bevelled, sitting on its own shadow.
*/
inline void raisedSlab (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, const SlabStyle& style = {})
{
    if (bounds.getWidth() < 2.0f || bounds.getHeight() < 2.0f) return;

    if (style.shadow > 0.01f)
        contactShadow (g, bounds, corner, juce::jlimit (3.0f, 16.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.055f), style.shadow);

    // A hard dark line just outside the slab separates it from the chassis.
    g.setColour (Theme::panelEdge.withAlpha (0.85f));
    g.drawRoundedRectangle (bounds.expanded (0.5f), corner + 0.5f, 1.0f);

    juce::ColourGradient body (style.top, bounds.getCentreX(), bounds.getY(), style.bottom, bounds.getCentreX(), bounds.getBottom(), false);
    body.addColour (0.5, style.top.interpolatedWith (style.bottom, 0.68f));
    g.setGradientFill (body);
    g.fillRoundedRectangle (bounds, corner);

    brushedStreaks (g, bounds, corner, 0.42f * style.brush);

    if (style.sheen > 0.01f)
    {
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.05f * style.sheen), bounds.getX(), bounds.getY(),
                                    juce::Colours::transparentWhite,
                                    bounds.getX() + bounds.getWidth() * 0.55f, bounds.getY() + bounds.getHeight() * 0.8f, false);
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (bounds, corner);
    }

    bevelEdge (g, bounds, corner, 0.17f * style.bevel, 0.62f * style.bevel, 1.3f);

    // A crisp highlight just inside the top edge finishes the machined lip.
    if (style.bevel > 0.01f && bounds.getWidth() > corner * 2.5f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.075f * style.bevel));
        g.drawLine (bounds.getX() + corner * 0.85f, bounds.getY() + 1.6f, bounds.getRight() - corner * 0.85f, bounds.getY() + 1.6f, 1.0f);
    }
}

/**
    An inset well: the same slab with the light inverted, so it reads as a
    recess cut into the panel — the screens sit behind glass in these.
*/
inline void insetWell (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, juce::Colour fill = Theme::panelInset, float depth = 1.0f)
{
    if (bounds.getWidth() < 2.0f || bounds.getHeight() < 2.0f) return;

    juce::ColourGradient body (fill.darker (0.35f), bounds.getCentreX(), bounds.getY(),
                               fill.brighter (0.13f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (bounds, corner);

    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds, corner);
        g.reduceClipRegion (clip);

        // Shadow cast by the top and left lips of the recess.
        const float dy = juce::jmin (bounds.getHeight() * 0.45f, 20.0f) * depth;
        juce::ColourGradient top (juce::Colours::black.withAlpha (0.60f * depth), bounds.getX(), bounds.getY(),
                                  juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + dy, false);
        g.setGradientFill (top);
        g.fillRect (bounds.withHeight (dy));

        const float dx = juce::jmin (bounds.getWidth() * 0.4f, 16.0f) * depth;
        juce::ColourGradient left (juce::Colours::black.withAlpha (0.40f * depth), bounds.getX(), bounds.getY(),
                                   juce::Colours::transparentBlack, bounds.getX() + dx, bounds.getY(), false);
        g.setGradientFill (left);
        g.fillRect (bounds.withWidth (dx));
    }

    // Inverted bevel: dark at the top-left, a lit lower-right lip.
    juce::Path p;
    p.addRoundedRectangle (bounds.reduced (0.6f), juce::jmax (0.0f, corner - 0.6f));
    juce::ColourGradient edge (juce::Colours::black.withAlpha (0.80f * depth), bounds.getX(), bounds.getY(),
                               juce::Colours::white.withAlpha (0.13f), bounds.getRight(), bounds.getBottom(), false);
    edge.addColour (0.45, juce::Colours::transparentBlack);
    g.setGradientFill (edge);
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

/**
    A screw head: dark, seated in the panel, with a bright crescent on its
    upper-left and a slot cut across it. Small and quiet, four to a panel.
*/
inline void screw (juce::Graphics& g, juce::Point<float> centre, float radius, float angleRadians = 0.6f, float brightness = 1.0f)
{
    if (radius < 1.1f) return;
    const juce::Rectangle<float> head (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // The seat it is sunk into.
    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillEllipse (head.expanded (radius * 0.30f).translated (radius * 0.10f, radius * 0.18f));

    juce::ColourGradient body (Theme::metal.withMultipliedBrightness (0.85f * brightness), centre.x + kLightX * radius * 0.7f, centre.y + kLightY * radius * 0.65f,
                               juce::Colour (0xff0b0c10), centre.x - kLightX * radius * 1.2f, centre.y - kLightY * radius * 1.1f, true);
    g.setGradientFill (body);
    g.fillEllipse (head);

    // Bright crescent on the lit side.
    juce::Path crescent;
    crescent.addCentredArc (centre.x, centre.y, radius * 0.78f, radius * 0.78f, 0.0f,
                            -juce::MathConstants<float>::pi * 0.92f, -juce::MathConstants<float>::pi * 0.08f, true);
    g.setColour (juce::Colours::white.withAlpha (0.28f * brightness));
    g.strokePath (crescent, juce::PathStrokeType (juce::jmax (0.7f, radius * 0.3f)));

    // The slot: cut in, so it is dark with a lit lower lip.
    const float c = std::cos (angleRadians), s = std::sin (angleRadians);
    const juce::Point<float> a (centre.x - c * radius * 0.6f, centre.y - s * radius * 0.6f);
    const juce::Point<float> b (centre.x + c * radius * 0.6f, centre.y + s * radius * 0.6f);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawLine ({ a, b }, juce::jmax (0.8f, radius * 0.28f));
    g.setColour (juce::Colours::white.withAlpha (0.18f * brightness));
    g.drawLine (a.x, a.y + radius * 0.26f, b.x, b.y + radius * 0.26f, juce::jmax (0.6f, radius * 0.15f));

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (head.reduced (0.3f), juce::jmax (0.6f, radius * 0.14f));
}

/** The four corner screws that bolt a panel to the chassis. */
inline void rivets (juce::Graphics& g, juce::Rectangle<float> bounds, float inset, float radius, float brightness = 1.0f)
{
    if (radius < 1.1f || bounds.getWidth() < inset * 4.0f || bounds.getHeight() < inset * 4.0f) return;
    const float x0 = bounds.getX() + inset, x1 = bounds.getRight() - inset;
    const float y0 = bounds.getY() + inset, y1 = bounds.getBottom() - inset;
    screw (g, { x0, y0 }, radius,  0.55f, brightness);
    screw (g, { x1, y0 }, radius, -0.70f, brightness);
    screw (g, { x0, y1 }, radius,  1.20f, brightness);
    screw (g, { x1, y1 }, radius,  0.20f, brightness);
}

/** The chassis behind everything: warm charcoal, grained, with the corners falling away. */
inline void chassisBackground (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::ColourGradient base (Theme::backgroundTop, bounds.getCentreX(), bounds.getY(),
                               Theme::background, bounds.getCentreX(), bounds.getBottom(), false);
    base.addColour (0.35, Theme::backgroundTop.interpolatedWith (Theme::background, 0.55f));
    g.setGradientFill (base);
    g.fillRect (bounds);

    grain (g, bounds, 0.85f);

    // Vignette: four edge gradients, so it stays proportional at any aspect ratio.
    const float fx = bounds.getWidth() * 0.24f, fy = bounds.getHeight() * 0.22f;
    const auto dark = juce::Colours::black.withAlpha (0.42f);
    auto edge = [&g, dark] (juce::Rectangle<float> area, float x1, float y1, float x2, float y2)
    {
        juce::ColourGradient grad (dark, x1, y1, juce::Colours::transparentBlack, x2, y2, false);
        g.setGradientFill (grad);
        g.fillRect (area);
    };
    edge (bounds.withHeight (fy), bounds.getX(), bounds.getY(), bounds.getX(), bounds.getY() + fy);
    edge (bounds.withTop (bounds.getBottom() - fy), bounds.getX(), bounds.getBottom(), bounds.getX(), bounds.getBottom() - fy);
    edge (bounds.withWidth (fx), bounds.getX(), bounds.getY(), bounds.getX() + fx, bounds.getY());
    edge (bounds.withLeft (bounds.getRight() - fx), bounds.getRight(), bounds.getY(), bounds.getRight() - fx, bounds.getY());
}

/** Panel surface (the standard slab). */
inline void panelSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    raisedSlab (g, bounds, corner);
}

/** Inset (sunken) area inside a panel, e.g. behind a waveform. */
inline void insetSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    insetWell (g, bounds, corner);
}

//==============================================================================
// Round bodies
//==============================================================================

/**
    A moulded dome: the body shared by knobs, slider handles and the source
    spheres. Lit from the top-left, with a machined rim and a specular bloom.
*/
inline void domeBody (juce::Graphics& g, juce::Rectangle<float> circle, juce::Colour base, float lit = 0.0f, float shadow = 1.0f)
{
    const auto c = circle.getCentre();
    const float r = circle.getWidth() * 0.5f;
    if (r < 1.0f) return;

    if (shadow > 0.01f)
        contactShadowEllipse (g, circle, juce::jlimit (2.0f, 14.0f, r * 0.24f), shadow);

    // The dome: brightest where the light strikes it, falling away to the lower right.
    juce::ColourGradient body (base.brighter (0.85f + 0.35f * lit), c.x + kLightX * r * 0.62f, c.y + kLightY * r * 0.58f,
                               base.darker (0.80f), c.x - kLightX * r * 0.95f, c.y - kLightY * r * 0.92f, true);
    body.addColour (0.42, base.brighter (0.24f + 0.12f * lit));
    body.addColour (0.72, base.darker (0.35f));
    g.setGradientFill (body);
    g.fillEllipse (circle);

    // Ambient bounce along the lower-right edge keeps the dome from going flat black.
    {
        juce::ColourGradient bounce (juce::Colours::transparentBlack, c.x, c.y,
                                     base.brighter (0.42f).withAlpha (0.34f), c.x - kLightX * r, c.y - kLightY * r, true);
        bounce.addColour (0.74, juce::Colours::transparentBlack);
        g.setGradientFill (bounce);
        g.fillEllipse (circle);
    }

    // Machined rim: a bright crescent centred on the light, dark around the shaded edge.
    {
        const float rimW = juce::jmax (0.9f, r * 0.055f);
        const float toLight = std::atan2 (kLightX, -kLightY);   // 0 rad is 12 o'clock, clockwise
        juce::Path lip;
        lip.addCentredArc (c.x, c.y, r - rimW * 0.6f, r - rimW * 0.6f, 0.0f, toLight - 1.45f, toLight + 1.45f, true);
        juce::ColourGradient shine (juce::Colours::white.withAlpha (0.34f + 0.26f * lit), c.x, c.y - r,
                                    juce::Colours::white.withAlpha (0.05f), c.x, c.y + r, false);
        g.setGradientFill (shine);
        g.strokePath (lip, juce::PathStrokeType (rimW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path shade;
        shade.addCentredArc (c.x, c.y, r - rimW * 0.6f, r - rimW * 0.6f, 0.0f,
                             toLight + 1.45f, toLight + juce::MathConstants<float>::twoPi - 1.45f, true);
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.strokePath (shade, juce::PathStrokeType (rimW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Specular bloom in the upper-left third: broad, then a tighter core.
    softLight (g, { c.x + kLightX * r * 0.46f, c.y + kLightY * r * 0.42f }, r * 0.74f, juce::Colours::white, 0.11f + 0.07f * lit);
    {
        // A soft ellipse, squashed along the light direction, is the sheen itself.
        const juce::Point<float> sc (c.x + kLightX * r * 0.40f, c.y + kLightY * r * 0.36f);
        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (juce::AffineTransform::rotation (-0.62f, sc.x, sc.y).scaled (1.0f, 0.62f, sc.x, sc.y));
        softLight (g, sc, r * 0.52f, juce::Colours::white, 0.14f + 0.10f * lit);
    }

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (circle, 1.0f);
}

/** Sphere-like dark base used by knobs and selectors. */
inline void sphere (juce::Graphics& g, juce::Rectangle<float> circle, float lit = 0.0f)
{
    domeBody (g, circle, Theme::knobBase, lit, 1.0f);
}

/** Thin arc used by knobs and rings. */
inline juce::Path arc (juce::Rectangle<float> bounds, float startRadians, float endRadians)
{
    juce::Path p;
    p.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f,
                     0.0f, startRadians, endRadians, true);
    return p;
}

/**
    Strokes an arc with the two-stop gradient of a section's accent pair.

    The gradient runs left to right across the control, which is the direction
    the sweep travels: the first colour sits at 7 o'clock, the second at 5.
*/
inline void gradientArc (juce::Graphics& g, juce::Rectangle<float> bounds, float from, float to,
                         juce::Colour first, juce::Colour second, float width, float alpha = 1.0f)
{
    if (to <= from + 0.0005f || width <= 0.0f) return;
    juce::ColourGradient grad (first.withMultipliedAlpha (alpha), bounds.getX(), bounds.getCentreY(),
                               second.withMultipliedAlpha (alpha), bounds.getRight(), bounds.getCentreY(), false);
    g.setGradientFill (grad);
    g.strokePath (arc (bounds, from, to), juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

/** Fills a rounded rectangle with a section's accent pair, left to right. */
inline void gradientCapsule (juce::Graphics& g, juce::Rectangle<float> bounds, float corner,
                             juce::Colour first, juce::Colour second, float alpha = 1.0f)
{
    if (bounds.isEmpty()) return;
    juce::ColourGradient grad (first.withMultipliedAlpha (alpha), bounds.getX(), bounds.getCentreY(),
                               second.withMultipliedAlpha (alpha), bounds.getRight(), bounds.getCentreY(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, corner);
}

//==============================================================================
// Glyphs and text
//==============================================================================

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

/** Builds the outline of one line of text, positioned in `area` by `just`. */
inline juce::Path textPath (const juce::String& text, juce::Rectangle<float> area, juce::Justification just, const juce::Font& font)
{
    juce::Path p;
    if (text.isEmpty()) return p;

    juce::GlyphArrangement ga;
    ga.addLineOfText (font, text, 0.0f, 0.0f);
    ga.createPath (p);
    const auto box = p.getBounds();
    if (box.isEmpty()) return p;

    float x = area.getX() - box.getX();
    if (just.testFlags (juce::Justification::horizontallyCentred)) x = area.getCentreX() - box.getCentreX();
    else if (just.testFlags (juce::Justification::right))          x = area.getRight() - box.getRight();

    float y = area.getCentreY() - box.getCentreY();
    if (just.testFlags (juce::Justification::top))         y = area.getY() - box.getY();
    else if (just.testFlags (juce::Justification::bottom)) y = area.getBottom() - box.getBottom();

    p.applyTransform (juce::AffineTransform::translation (x, y));
    return p;
}

/**
    Machined lettering: filled with a vertical metal gradient, with a light
    top edge and a dark bottom edge — the wordmark and its like.
*/
inline void metallicText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                          const juce::Font& font, juce::Colour tint = Theme::textPrimary, float bevelScale = 1.0f)
{
    const auto path = textPath (text, area, just, font);
    const auto box = path.getBounds();
    if (box.isEmpty()) return;

    const float bevel = juce::jmax (0.5f, font.getHeight() * 0.05f) * bevelScale;

    // Dark edge underneath, light edge above: the two halves of the bevel.
    g.setColour (juce::Colours::black.withAlpha (0.70f));
    g.fillPath (path, juce::AffineTransform::translation (bevel * 0.35f, bevel));
    g.setColour (tint.brighter (0.9f).withAlpha (0.35f));
    g.fillPath (path, juce::AffineTransform::translation (-bevel * 0.3f, -bevel * 0.75f));

    juce::ColourGradient metal (tint.brighter (0.60f), box.getCentreX(), box.getY(),
                                tint.darker (0.42f), box.getCentreX(), box.getBottom(), false);
    metal.addColour (0.44, tint.brighter (0.12f));
    metal.addColour (0.56, tint.darker (0.14f));
    g.setGradientFill (metal);
    g.fillPath (path);
}

/**
    Engraved lettering: cut into the metal, so it is dark with a lit lower lip
    and no bright fill of its own.
*/
inline void engravedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                          const juce::Font& font, juce::Colour surface, float strength = 1.0f)
{
    const auto path = textPath (text, area, just, font);
    if (path.isEmpty()) return;
    const float lip = juce::jmax (0.6f, font.getHeight() * 0.07f);

    g.setColour (surface.brighter (0.85f).withAlpha (0.34f * strength));
    g.fillPath (path, juce::AffineTransform::translation (0.0f, lip));
    g.setColour (surface.darker (0.9f).withAlpha (0.92f * strength));
    g.fillPath (path);
}

} // namespace am::ui::draw
