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

    The ground is pale, and that changes where depth comes from. A panel cannot
    be lighter than the chassis by much — it is frosted glass on pearl, and the
    two are within a few percent of each other — so the thing that lifts it off
    the ground is the **shadow it casts**, below and slightly right, plus one
    near-white pixel along its top-left edge and a faint dark one along its
    bottom-right. Nothing is outlined. The single dark element is the recessed
    display, and it is genuinely black.

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
                    // Balanced on a pale ground: dark speckle alone turns pearl to concrete.
                    const float a = std::abs (v) * (v < 0.0f ? 0.040f : 0.034f);
                    data.setPixelColour (x, y, (v < 0.0f ? juce::Colours::black : juce::Colours::white).withAlpha (a));
                }
            return img;
        }();
        return tile;
    }

    /**
        Frost on a glass panel: a fine speckle over a soft low-frequency mottle,
        mostly white. Built once and tiled; the lattice wraps, so there is no seam.

        A panel face is nearly white, so this has to be a whisper — the point is
        only that a large slab never looks like flat paint.
    */
    inline const juce::Image& frostTile()
    {
        static const juce::Image tile = []
        {
            constexpr int n = 128, cells = 8;
            juce::Random rng (0xf0517e);
            float lattice[cells][cells];
            for (auto& row : lattice)
                for (auto& v : row) v = rng.nextFloat();

            auto smooth = [] (float t) { return t * t * (3.0f - 2.0f * t); };

            juce::Image img (juce::Image::ARGB, n, n, true);
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < n; ++y)
                for (int x = 0; x < n; ++x)
                {
                    const float fx = (float) x / (float) n * (float) cells;
                    const float fy = (float) y / (float) n * (float) cells;
                    const int x0 = (int) fx, y0 = (int) fy;
                    const int x1 = (x0 + 1) % cells, y1 = (y0 + 1) % cells;
                    const float sx = smooth (fx - (float) x0), sy = smooth (fy - (float) y0);
                    const float a = lattice[y0][x0] + (lattice[y0][x1] - lattice[y0][x0]) * sx;
                    const float b = lattice[y1][x0] + (lattice[y1][x1] - lattice[y1][x0]) * sx;
                    const float cloud = (a + (b - a) * sy) * 2.0f - 1.0f;
                    const float speckle = rng.nextFloat() * 2.0f - 1.0f;
                    const float v = cloud * 0.45f + speckle * 0.55f;
                    data.setPixelColour (x, y, (v > 0.0f ? juce::Colours::white : juce::Colours::black)
                                                   .withAlpha (std::abs (v) * (v > 0.0f ? 0.080f : 0.042f)));
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

/** A whisper of frost across the face of a glass panel, clipped to its rounded rectangle. */
inline void frost (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float strength = 1.0f)
{
    if (strength <= 0.01f || bounds.getWidth() < 3.0f || bounds.getHeight() < 3.0f) return;
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip;
    clip.addRoundedRectangle (bounds, corner);
    g.reduceClipRegion (clip);
    g.setTiledImageFill (detail::frostTile(), juce::roundToInt (bounds.getX()), juce::roundToInt (bounds.getY()),
                         juce::jlimit (0.0f, 1.0f, strength));
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

/**
    The soft drop shadow a floating slab casts: below and slightly right, in the
    single light direction, reaching exactly `reach` past the shape.

    This is the primitive the whole light-ground design rests on. A panel is not
    brighter than the pearl it sits on — it is the shadow underneath that says it
    is above the chassis rather than cut into it. So it is wide and very soft
    (about 18 % black where it is deepest, spread over a dozen rings) rather than
    the short hard crescent a dark ground could get away with.
*/
inline void dropShadow (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float reach, float strength = 1.0f)
{
    if (reach < 0.5f || strength <= 0.01f || bounds.isEmpty()) return;
    const int steps = juce::jlimit (4, 14, (int) (reach * 1.4f));
    const float dx = reach * 0.22f, dy = reach * 0.44f;   // the offset: down, and a little right
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float spread = reach * 0.56f * t;
        const float alpha = 0.050f * strength * (1.0f - t) * (1.0f - t) + 0.004f * strength;
        g.setColour (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.fillRoundedRectangle (bounds.expanded (spread).translated (dx, dy), corner + spread);
    }
}

/** Contact shadow beneath a rounded shape: shorter and tighter than a drop shadow. */
inline void contactShadow (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float radius, float strength = 1.0f)
{
    dropShadow (g, bounds, corner, radius, strength);
}

/**
    Contact shadow beneath a circular control.

    The reach is deliberately short and grows slowly, because a shadow that
    scales with the control turns into a hard crescent offset from it — a
    second, misaligned ring rather than the thing sitting on the panel.
*/
inline void contactShadowEllipse (juce::Graphics& g, juce::Rectangle<float> circle, float radius, float strength = 1.0f)
{
    if (radius < 0.5f || strength <= 0.01f) return;
    const int steps = juce::jlimit (4, 12, (int) (radius * 1.6f));
    const float dx = radius * 0.24f, dy = radius * 0.46f;
    for (int i = steps; i >= 1; --i)
    {
        const float t = (float) i / (float) steps;
        const float spread = radius * 0.6f * t;
        g.setColour (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, 0.048f * strength * (1.0f - t) * (1.0f - t) + 0.005f * strength)));
        g.fillEllipse (circle.expanded (spread).translated (dx, dy));
    }
}

/**
    The shadow the top and left lips of a recess throw into it, clipped to the
    recess itself. The counterpart of `dropShadow`: same light, other side.
*/
inline void innerShadow (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float depth, float strength)
{
    if (strength <= 0.005f || bounds.getWidth() < 2.0f || bounds.getHeight() < 2.0f) return;
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip;
    clip.addRoundedRectangle (bounds, corner);
    g.reduceClipRegion (clip);

    const float dy = juce::jmin (bounds.getHeight() * 0.5f, depth);
    juce::ColourGradient top (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, strength)), bounds.getX(), bounds.getY(),
                              juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + dy, false);
    top.addColour (0.35, juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, strength * 0.30f)));
    g.setGradientFill (top);
    g.fillRect (bounds.withHeight (dy));

    const float dx = juce::jmin (bounds.getWidth() * 0.45f, depth * 0.8f);
    juce::ColourGradient left (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, strength * 0.55f)), bounds.getX(), bounds.getY(),
                               juce::Colours::transparentBlack, bounds.getX() + dx, bounds.getY(), false);
    g.setGradientFill (left);
    g.fillRect (bounds.withWidth (dx));
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

    // The axis leans with the light but stays mostly vertical, so the whole of the
    // top edge is lit and the whole of the bottom edge is in shadow — a strictly
    // diagonal axis fades the top edge out before it reaches the right-hand corner.
    juce::ColourGradient grad (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 1.0f, light)),
                               bounds.getX() + bounds.getWidth() * 0.18f, bounds.getY(),
                               juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, dark)),
                               bounds.getX() + bounds.getWidth() * 0.82f, bounds.getBottom(), false);
    grad.addColour (0.40, juce::Colours::white.withAlpha (juce::jlimit (0.0f, 1.0f, light * 0.22f)));
    grad.addColour (0.62, juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, dark * 0.18f)));
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
    float shadow = 1.0f;   ///< the drop shadow the slab floats on — this is what makes it read
    float shadowRadius = 0.0f;  ///< how far that shadow reaches (0 chooses from the size)
    float bevel  = 1.0f;   ///< near-white top-left edge, faint dark bottom-right edge
    float brush  = 1.0f;   ///< the texture on the face: frost on glass
    float sheen  = 1.0f;   ///< broad diagonal light from the top-left
};

/**
    A raised slab: the panel material — a slab of frosted white glass floating
    above the chassis.

    It is built from four passes and nothing else:
      1. a soft drop shadow beneath and slightly right,
      2. a nearly white vertical gradient, lighter at the top,
      3. a whisper of frost across the face and a broad diagonal sheen,
      4. one near-white pixel along the top and left edges and a faint dark
         hairline along the bottom and right.

    There is deliberately no outline: an outlined panel reads as a sticker on a
    light ground. The slab is never darker than the chassis — if it looks flat,
    the shadow is too small, not the fill too bright.
*/
inline void raisedSlab (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, const SlabStyle& style = {})
{
    if (bounds.getWidth() < 2.0f || bounds.getHeight() < 2.0f) return;

    if (style.shadow > 0.01f)
        dropShadow (g, bounds, corner,
                    style.shadowRadius > 0.0f ? style.shadowRadius
                                              : juce::jlimit (3.0f, 12.0f, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.05f),
                    style.shadow);

    juce::ColourGradient body (style.top, bounds.getCentreX(), bounds.getY(), style.bottom, bounds.getCentreX(), bounds.getBottom(), false);
    body.addColour (0.55, style.top.interpolatedWith (style.bottom, 0.55f));
    g.setGradientFill (body);
    g.fillRoundedRectangle (bounds, corner);

    frost (g, bounds, corner, 0.5f * style.brush);

    if (style.sheen > 0.01f)
    {
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.5f * style.sheen), bounds.getX(), bounds.getY(),
                                    juce::Colours::transparentWhite,
                                    bounds.getX() + bounds.getWidth() * 0.62f, bounds.getY() + bounds.getHeight() * 0.9f, false);
        sheen.addColour (0.45, juce::Colours::white.withAlpha (0.10f * style.sheen));
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (bounds, corner);
    }

    // The lit edge is strong and the shadowed one is faint: on a pale ground a dark
    // edge all the way round would read as an outline drawn on top of the glass.
    bevelEdge (g, bounds, corner, 0.95f * style.bevel, 0.20f * style.bevel, 1.2f);
}

/**
    An inset well: the same slab with the light inverted, so it reads as a recess
    cut into the panel.

    It does two jobs, and which one it does depends on how dark the fill is:

      * a **light capsule** (the default `Theme::panelInset`) — a slider track, a
        segment strip, a stepper pill. Slightly darker than the panel it is cut
        into, with a gentle shadow under its top lip and a white catch along the
        bottom. Quiet: the thing that sits *in* it is the subject.

      * a **display** (anything near-black, the default of `screenWell`) — the one
        dark element on the page. The mouth of the cut is a hard dark hairline at
        the top-left and a bright lit lip at the bottom-right, and the shadow the
        top lip throws inside is deep. That, over near-black, is what makes a
        screen look milled into the panel rather than painted on it.
*/
inline void insetWell (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, juce::Colour fill = Theme::panelInset, float depth = 1.0f)
{
    if (bounds.getWidth() < 2.0f || bounds.getHeight() < 2.0f) return;
    const bool screen = fill.getPerceivedBrightness() < 0.4f;

    // Darkest under the top lip, lifting a little toward the bottom where the light
    // bounces back in off the panel. The light capsule keeps that ramp small: a
    // strong one turns a shallow groove into a chrome trough.
    juce::ColourGradient body (screen ? fill.darker (0.45f) : fill.darker (0.045f), bounds.getCentreX(), bounds.getY(),
                               screen ? fill.brighter (0.16f) : fill.brighter (0.045f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (bounds, corner);

    innerShadow (g, bounds, corner,
                 juce::jmin (bounds.getHeight() * (screen ? 0.45f : 0.3f), screen ? 26.0f : 5.0f),
                 (screen ? 0.72f : 0.22f) * depth);

    // The mouth of the cut: dark where it goes in at the top-left, lit where it
    // comes back up at the bottom-right.
    juce::Path p;
    p.addRoundedRectangle (bounds.reduced (0.6f), juce::jmax (0.0f, corner - 0.6f));
    juce::ColourGradient edge (juce::Colours::black.withAlpha (juce::jlimit (0.0f, 1.0f, (screen ? 0.85f : 0.28f) * depth)),
                               bounds.getX() + bounds.getWidth() * 0.2f, bounds.getY(),
                               juce::Colours::white.withAlpha (screen ? 0.42f : 0.90f),
                               bounds.getX() + bounds.getWidth() * 0.8f, bounds.getBottom(), false);
    edge.addColour (0.45, juce::Colours::transparentBlack);
    g.setGradientFill (edge);
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

/**
    A recessed near-black display — the signature element of the instrument
    (SPEC section 1). Luminous line art is drawn into it and `screenGlass` goes
    over the top.
*/
inline void screenWell (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, juce::Colour fill = Theme::screen, float depth = 1.0f)
{
    insetWell (g, bounds, corner, fill, depth);
}

/** A light capsule cut into a panel: slider tracks, segment strips, stepper pills. */
inline void capsuleTrack (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float depth = 1.0f)
{
    insetWell (g, bounds, corner, Theme::panelInset, depth);
}

/**
    A screw head, seated in the panel: a shallow dish with a slot cut across it.

    The reference has no visible fasteners on its panels, so nothing draws these
    any more; they survive as a primitive for hardware that genuinely is bolted
    down (the sphere bezel). On a pale panel a screw is a *dimple*, not a dark
    stud — light in the dish, a shadow under the upper lip, a lit lower one.
*/
inline void screw (juce::Graphics& g, juce::Point<float> centre, float radius, float angleRadians = 0.6f, float brightness = 1.0f)
{
    if (radius < 1.1f) return;
    const juce::Rectangle<float> head (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient body (Theme::metalLight.withMultipliedBrightness (brightness), centre.x - kLightX * radius * 0.6f, centre.y - kLightY * radius * 0.6f,
                               Theme::metalDark, centre.x + kLightX * radius * 1.1f, centre.y + kLightY * radius * 1.1f, false);
    g.setGradientFill (body);
    g.fillEllipse (head);

    // The slot: cut in, so it is dark with a lit lower lip.
    const float c = std::cos (angleRadians), s = std::sin (angleRadians);
    const juce::Point<float> a (centre.x - c * radius * 0.62f, centre.y - s * radius * 0.62f);
    const juce::Point<float> b (centre.x + c * radius * 0.62f, centre.y + s * radius * 0.62f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine ({ a, b }, juce::jmax (0.7f, radius * 0.24f));
    g.setColour (juce::Colours::white.withAlpha (0.45f * brightness));
    g.drawLine (a.x, a.y + radius * 0.24f, b.x, b.y + radius * 0.24f, juce::jmax (0.6f, radius * 0.14f));

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawEllipse (head.reduced (0.3f), juce::jmax (0.6f, radius * 0.12f));
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

/**
    The chassis behind everything: a cool pearl grey, very slightly lighter
    toward the top, with a fine machined grain.

    No vignette. The ground is light and open — darkening the corners on a pale
    chassis reads as dirt on the photograph rather than as depth, and it is the
    panel shadows, not the background, that carry the depth here.
*/
inline void chassisBackground (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::ColourGradient base (Theme::backgroundTop, bounds.getCentreX(), bounds.getY(),
                               Theme::background, bounds.getCentreX(), bounds.getBottom(), false);
    base.addColour (0.45, Theme::backgroundTop.interpolatedWith (Theme::background, 0.52f));
    g.setGradientFill (base);
    g.fillRect (bounds);

    // The one lamp, above and to the left of the instrument: a very broad, very
    // soft lift, so the pearl has a direction without having an edge.
    softLight (g, { bounds.getX() + bounds.getWidth() * 0.24f, bounds.getY() - bounds.getHeight() * 0.18f },
               juce::jmax (bounds.getWidth(), bounds.getHeight()) * 0.95f, juce::Colours::white, 0.20f);

    grain (g, bounds, 0.9f);
}

/**
    The glass over a recessed screen: a broad diagonal sweep from the top-left,
    a tight highlight along the inside of the top edge and a faint thickening
    toward the lower right. A highlight layer — it must never haze the display.
*/
inline void screenGlass (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float strength = 1.0f)
{
    if (strength <= 0.01f || bounds.getWidth() < 4.0f || bounds.getHeight() < 4.0f) return;
    juce::Graphics::ScopedSaveState save (g);
    juce::Path clip;
    clip.addRoundedRectangle (bounds, corner);
    g.reduceClipRegion (clip);

    // The shadow the top lip of the recess throws across the screen, over the line
    // art as well as the ground: without it a dark rectangle is a hole, not a well.
    const float drop = juce::jmin (bounds.getHeight() * 0.26f, 18.0f);
    juce::ColourGradient inner (juce::Colours::black.withAlpha (0.42f * strength), bounds.getX(), bounds.getY(),
                                juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + drop, false);
    inner.addColour (0.4, juce::Colours::black.withAlpha (0.12f * strength));
    g.setGradientFill (inner);
    g.fillRect (bounds.withHeight (drop));

    juce::ColourGradient sweep (juce::Colours::white.withAlpha (0.055f * strength), bounds.getX(), bounds.getY(),
                                juce::Colours::transparentWhite,
                                bounds.getX() + bounds.getWidth() * 0.62f, bounds.getY() + bounds.getHeight() * 0.85f, false);
    sweep.addColour (0.35, juce::Colours::white.withAlpha (0.018f * strength));
    g.setGradientFill (sweep);
    g.fillRect (bounds);

    juce::ColourGradient thickness (juce::Colours::transparentBlack, bounds.getX(), bounds.getY(),
                                    juce::Colours::black.withAlpha (0.25f * strength), bounds.getRight(), bounds.getBottom(), false);
    thickness.addColour (0.55, juce::Colours::transparentBlack);
    g.setGradientFill (thickness);
    g.fillRect (bounds);

    // A bright crescent on the inside of the top edge: the glass itself.
    const float lip = juce::jmin (bounds.getHeight() * 0.16f, 8.0f);
    juce::ColourGradient crest (juce::Colours::white.withAlpha (0.11f * strength), bounds.getX(), bounds.getY() + 1.0f,
                                juce::Colours::transparentWhite, bounds.getX(), bounds.getY() + lip, false);
    g.setGradientFill (crest);
    g.fillRect (bounds.withHeight (lip).reduced (corner * 0.5f, 0.0f));
}

/** Panel surface (the standard slab). */
inline void panelSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    raisedSlab (g, bounds, corner);
}

/** Inset (sunken) area inside a panel: a display, so near-black and deeply recessed. */
inline void insetSurface (juce::Graphics& g, juce::Rectangle<float> bounds, float corner)
{
    screenWell (g, bounds, corner);
}

//==============================================================================
// Selection
//
// Three shapes carry selection across the instrument and they are deliberately
// the same object at three sizes: a white slab lifted out of a recess, marked in
// the section colour. Build new selectable things out of these rather than
// inventing a fourth.
//==============================================================================

/** The short accent bar that marks a selected thing. */
inline void accentUnderline (juce::Graphics& g, juce::Rectangle<float> line, juce::Colour first, juce::Colour second, float amount = 1.0f)
{
    if (line.isEmpty() || amount <= 0.01f) return;
    juce::ColourGradient grad (first.withMultipliedAlpha (amount), line.getX(), line.getCentreY(),
                               second.withMultipliedAlpha (amount), line.getRight(), line.getCentreY(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (line, juce::jmin (line.getHeight(), line.getWidth()) * 0.5f);
}

/**
    A key: the small raised slab a button, a tab or a stepper is made of. White at
    the top, floating on its own shadow; `lit` brightens it for hover.
*/
inline void keySlab (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, float lit = 0.0f, float shadow = 0.85f,
                     float shadowRadius = 0.0f)
{
    SlabStyle style;
    style.top    = juce::Colours::white;
    style.bottom = Theme::panelTop.interpolatedWith (Theme::panel, 0.85f - 0.55f * juce::jlimit (0.0f, 1.0f, lit));
    style.shadow = shadow;
    style.shadowRadius = shadowRadius;   // 0 chooses from the size; pass the margin you have
    style.brush  = 0.45f;
    style.sheen  = 0.9f;
    raisedSlab (g, bounds, corner, style);
}

/**
    The selected cell of a segment, a tab or an option row (SPEC section 3): a
    raised white slab out of the recessed strip, with the accent as a thin
    underline and a soft glow beneath it.
*/
inline void selectedCell (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, juce::Colour accent,
                          float amount = 1.0f, bool underline = true, float shadowRadius = 0.0f)
{
    if (amount <= 0.01f || bounds.getWidth() < 3.0f || bounds.getHeight() < 3.0f) return;
    const auto pair = Theme::accentPair (accent);

    glowRoundedRect (g, bounds, corner, pair.second, juce::jmin (bounds.getHeight() * 0.5f, 10.0f), 0.35f * amount);
    keySlab (g, bounds, corner, 0.55f, 0.9f * amount, shadowRadius);

    if (! underline) return;
    const float h = juce::jlimit (1.5f, 3.0f, bounds.getHeight() * 0.055f);
    const float inset = juce::jmin (bounds.getWidth() * 0.24f, corner + bounds.getWidth() * 0.06f);
    accentUnderline (g, { bounds.getX() + inset, bounds.getBottom() - h - juce::jmax (1.5f, corner * 0.25f),
                          bounds.getWidth() - inset * 2.0f, h }, pair.first, pair.second, amount);
}

/**
    A sidebar pill (SPEC section 3): the rows of the vertical lists on the SOURCE,
    SHAPE, EVOLVE, FRACTURE and SPACE pages. The selected pill is a raised white
    slab with the accent down its left edge; the rest sit flat and quiet.

    Draws the chrome only — the caller puts its own glyph and label on top, and
    `selected` / `hover` are eased values so a change of selection reads as motion.
*/
inline void sidebarPill (juce::Graphics& g, juce::Rectangle<float> bounds, float corner, juce::Colour accent,
                         float selected, float hover = 0.0f, float shadowRadius = 0.0f)
{
    if (bounds.getWidth() < 4.0f || bounds.getHeight() < 4.0f) return;
    const auto pair = Theme::accentPair (accent);
    const float hv = juce::jlimit (0.0f, 1.0f, hover) * (1.0f - juce::jlimit (0.0f, 1.0f, selected));

    if (hv > 0.02f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f * hv));
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (pair.second.withAlpha (0.16f * hv));
        g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
    }

    if (selected <= 0.02f) return;
    keySlab (g, bounds, corner, 0.5f, 0.95f * selected, shadowRadius);

    // The accent runs down the left edge, which is the edge the eye tracks in a
    // vertical list — an underline would be lost between the rows.
    const float w = juce::jlimit (2.0f, 4.0f, bounds.getHeight() * 0.09f);
    auto bar = bounds.withWidth (w).reduced (0.0f, juce::jmin (corner * 0.6f, bounds.getHeight() * 0.22f));
    glowRoundedRect (g, bar, w * 0.5f, pair.second, bounds.getHeight() * 0.30f, 0.40f * selected);
    juce::ColourGradient grad (pair.first.withMultipliedAlpha (selected), bar.getX(), bar.getY(),
                               pair.second.withMultipliedAlpha (selected), bar.getX(), bar.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bar, w * 0.5f);
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
        contactShadowEllipse (g, circle, juce::jlimit (2.0f, 6.5f, r * 0.16f), shadow);

    // The dome. The light strikes the upper left of the cap and the surface turns
    // away from it toward the lower right, so the two sides must be a long way
    // apart in tone: anything gentler reads as a hole in the panel, not a cap on it.
    const juce::Point<float> litPoint (c.x + kLightX * r * 0.58f, c.y + kLightY * r * 0.54f);
    const juce::Point<float> awayPoint (c.x - kLightX * r * 1.05f, c.y - kLightY * r * 1.02f);

    juce::ColourGradient body (base.brighter (1.05f + 0.40f * lit), litPoint.x, litPoint.y,
                               juce::Colour (0xff06070c), awayPoint.x, awayPoint.y, true);
    body.addColour (0.22, base.brighter (0.50f + 0.16f * lit));
    body.addColour (0.46, base.brighter (0.0f));
    body.addColour (0.70, base.darker (0.62f));
    g.setGradientFill (body);
    g.fillEllipse (circle);

    // Thickness: the far side of the cap turns away from the light and falls into
    // shadow. This is the pass that makes the body read as an object rather than a
    // disc, and it is what the glass spheres get too.
    {
        juce::ColourGradient depth (juce::Colours::transparentBlack, litPoint.x, litPoint.y,
                                    juce::Colours::black.withAlpha (0.70f), awayPoint.x, awayPoint.y, true);
        depth.addColour (0.40, juce::Colours::transparentBlack);
        depth.addColour (0.68, juce::Colours::black.withAlpha (0.26f));
        g.setGradientFill (depth);
        g.fillEllipse (circle);
    }

    // Ambient bounce along the lower-right edge keeps the dome from going flat black.
    {
        juce::ColourGradient bounce (juce::Colours::transparentBlack, c.x, c.y,
                                     base.brighter (0.65f).withAlpha (0.26f), c.x - kLightX * r, c.y - kLightY * r, true);
        bounce.addColour (0.80, juce::Colours::transparentBlack);
        g.setGradientFill (bounce);
        g.fillEllipse (circle);
    }

    // Machined rim: one continuous stroke lit on the light's side and dark on the
    // other, so the edge never breaks where the two halves meet.
    {
        const float rimW = juce::jmax (0.9f, r * 0.055f);
        juce::Path rim;
        rim.addEllipse (circle.reduced (rimW * 0.55f));
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.46f + 0.28f * lit), c.x + kLightX * r, c.y + kLightY * r,
                                   juce::Colours::black.withAlpha (0.88f), c.x - kLightX * r, c.y - kLightY * r, false);
        edge.addColour (0.38, juce::Colours::white.withAlpha (0.05f));
        edge.addColour (0.56, juce::Colours::black.withAlpha (0.10f));
        g.setGradientFill (edge);
        g.strokePath (rim, juce::PathStrokeType (rimW));

        // A hot line where the light actually catches the machined edge. It fades out
        // along the same axis, so the rim still reads as one continuous edge.
        juce::ColourGradient hot (juce::Colours::white.withAlpha (0.70f + 0.30f * lit), c.x + kLightX * r * 1.05f, c.y + kLightY * r * 1.05f,
                                  juce::Colours::transparentWhite, c.x - kLightX * r * 0.25f, c.y - kLightY * r * 0.25f, false);
        hot.addColour (0.55, juce::Colours::white.withAlpha (0.04f));
        g.setGradientFill (hot);
        g.strokePath (rim, juce::PathStrokeType (juce::jmax (0.8f, rimW * 0.45f)));
    }

    // Specular bloom in the upper-left third: a broad sheen, then a tight highlight.
    softLight (g, { c.x + kLightX * r * 0.44f, c.y + kLightY * r * 0.40f }, r * 0.78f, juce::Colours::white, 0.13f + 0.08f * lit);
    {
        // A soft ellipse, squashed and tilted along the light direction, is the sheen.
        const juce::Point<float> sc (c.x + kLightX * r * 0.46f, c.y + kLightY * r * 0.42f);
        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (juce::AffineTransform::rotation (-0.62f, sc.x, sc.y).scaled (1.0f, 0.52f, sc.x, sc.y));
        softLight (g, sc, r * 0.54f, juce::Colours::white, 0.20f + 0.10f * lit);
        // The highlight itself: small and defined, so the eye reads a surface.
        juce::ColourGradient core (juce::Colours::white.withAlpha (0.56f + 0.24f * lit), sc.x, sc.y,
                                   juce::Colours::transparentWhite, sc.x + r * 0.24f, sc.y, true);
        core.addColour (0.38, juce::Colours::white.withAlpha (0.30f + 0.14f * lit));
        core.addColour (0.74, juce::Colours::white.withAlpha (0.06f));
        g.setGradientFill (core);
        g.fillEllipse (sc.x - r * 0.24f, sc.y - r * 0.24f, r * 0.48f, r * 0.48f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.55f));
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

/**
    1 px hairline: a fine dark rule with a lit lip under it, so a divider on a
    pale panel reads as a scribed line rather than as a drawn border.
*/
inline void hairline (juce::Graphics& g, float x1, float y1, float x2, float y2, float alpha = 0.08f)
{
    g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 1.0f, alpha * 4.0f)));
    g.drawLine (x1, y1 + 1.0f, x2, y2 + 1.0f, 1.0f);
    g.setColour (Theme::textPrimary.withAlpha (juce::jlimit (0.0f, 1.0f, alpha * 1.1f)));
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
    A shape cut into the pale ground: a fine engraved bevel, a light edge along
    the top of the cut and a dark one along the bottom, with the face of the
    shape itself near-black (SPEC section 7).

    This replaces the metallic fill the wordmark used to carry. A gradient that
    ran from mid-grey to black was legible against charcoal and nearly invisible
    against pearl, because its lit half is the same value as the chassis.
*/
inline void engravedShape (juce::Graphics& g, const juce::Path& path, juce::Colour ink, float bevel, float strength = 1.0f)
{
    if (path.isEmpty()) return;
    bevel = juce::jmax (0.6f, bevel);

    g.setColour (juce::Colours::white.withAlpha (0.92f * strength));
    g.fillPath (path, juce::AffineTransform::translation (-bevel * 0.18f, -bevel));
    g.setColour (juce::Colours::black.withAlpha (0.20f * strength));
    g.fillPath (path, juce::AffineTransform::translation (bevel * 0.22f, bevel * 0.9f));

    const auto box = path.getBounds();
    juce::ColourGradient face (ink.brighter (0.10f), box.getCentreX(), box.getY(),
                               ink.darker (0.30f), box.getCentreX(), box.getBottom(), false);
    g.setGradientFill (face);
    g.fillPath (path);
}

/**
    Machined metal: a shape filled with a vertical metal gradient and given a
    fine bevel — a light edge along the top and a dark one along the bottom.

    Kept for chrome hardware (rims, bezels). Lettering on the pale ground uses
    `engravedShape` instead, which is what the wordmark wants.
*/
inline void metallicShape (juce::Graphics& g, const juce::Path& path, juce::Colour tint, float bevel)
{
    const auto box = path.getBounds();
    if (box.isEmpty()) return;
    bevel = juce::jmax (0.5f, bevel);

    // Dark edge underneath, light edge above: the two halves of the bevel.
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillPath (path, juce::AffineTransform::translation (bevel * 0.35f, bevel));
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillPath (path, juce::AffineTransform::translation (-bevel * 0.3f, -bevel * 0.75f));

    juce::ColourGradient metal (tint.brighter (0.30f), box.getCentreX(), box.getY(),
                                tint.darker (0.30f), box.getCentreX(), box.getBottom(), false);
    metal.addColour (0.46, tint.brighter (0.10f));
    metal.addColour (0.56, tint.darker (0.16f));
    g.setGradientFill (metal);
    g.fillPath (path);
}

/** Lettering cut into the pale ground — the wordmark and its like. */
inline void metallicText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                          const juce::Font& font, juce::Colour tint = Theme::textPrimary, float bevelScale = 1.0f)
{
    engravedShape (g, textPath (text, area, just, font), tint, juce::jmax (0.8f, font.getHeight() * 0.045f * bevelScale));
}

/**
    Engraved lettering: cut into the pale ground, with a lit top lip, a soft
    shadow along the bottom and a near-black face.
*/
inline void engravedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                          const juce::Font& font, juce::Colour ink = Theme::textPrimary, float strength = 1.0f)
{
    engravedShape (g, textPath (text, area, just, font), ink, juce::jmax (0.7f, font.getHeight() * 0.06f), strength);
}

} // namespace am::ui::draw
