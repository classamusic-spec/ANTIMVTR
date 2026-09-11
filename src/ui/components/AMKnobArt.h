#pragma once

#include "AMKnobGeometry.h"
#include "ui/AntiMatrTheme.h"

#include <cmath>
#include <cstdint>

namespace am::ui::knobart
{

/**
    The knob's own material language.

    ANTI-MATR's knobs are the one dark object on a pale chassis, and the whole
    family is three variants of a single piece of hardware (SPEC section 2):

      Style::CappedLit   a moulded body with a turned metal cap, ringed by LED
                         dots that light amber up to the value — the hero.
      Style::CappedDark  the same knob with every dot dark, for dense clusters
                         where a wall of glowing rings would be noise.
      Style::Plain       no cap and no dots: a quiet matte dome with a
                         concentric groove and a notch at the top.

    Everything here derives from the bounds it is handed, so one knob draws
    identically at 1100 x 690 and at 1600 x 1000, and everything obeys the same
    single light — above and slightly to the left — as the rest of the editor.

    These primitives are deliberately a private copy rather than an extension of
    `draw::`: the knobs' surfaces are tuned against each other (a cap's sheen has
    to sit a precise amount above its body's bevel) and must not drift when the
    shared material language is retuned for something else.
*/

/** The single light source, as a unit vector in component space. */
inline constexpr float kLightX = -0.5547f;
inline constexpr float kLightY = -0.8321f;

/** Bearing of that light, in the knobs' angle convention (0 = up, clockwise). */
inline const float kLightAngle = std::atan2 (kLightX, -kLightY);

//==============================================================================
/**
    Concentric geometry of one knob, from the rim inwards: the modulation arc,
    the ring of LED dots, a dark gap, the moulded body, and the metal cap inset
    into it. The modulation orbit is reserved whether or not the knob is
    modulated, so adding a routing never resizes anything.
*/
struct Geometry
{
    juce::Point<float> centre;
    float diameter = 0.0f;
    float startAngle = 0.0f, endAngle = 0.0f;

    float modRadius = 0.0f;   ///< radius of the modulation arc (outermost)
    float modStroke = 1.0f;   ///< its stroke width

    float ledRadius = 0.0f;   ///< the circle the LED dots sit on
    float dotRadius = 0.0f;   ///< radius of one dot
    int   dots = 0;           ///< 0 when the style carries no ring

    juce::Rectangle<float> body;   ///< the moulded body / matte dome
    juce::Rectangle<float> cap;    ///< the turned metal cap (empty when plain)
    bool capped = false;

    float bodyRadius() const noexcept { return body.getWidth() * 0.5f; }
    float capRadius()  const noexcept { return cap.getWidth() * 0.5f; }

    /** Angle of a normalised position along the sweep. */
    float angleAt (float t) const noexcept { return startAngle + juce::jlimit (0.0f, 1.0f, t) * (endAngle - startAngle); }

    juce::Point<float> polar (float radius, float angle) const noexcept
    {
        return { centre.x + std::sin (angle) * radius, centre.y - std::cos (angle) * radius };
    }

    /** Normalised position of dot `i` along the sweep. */
    float dotValue (int i) const noexcept { return dots > 1 ? (float) i / (float) (dots - 1) : 0.0f; }
    juce::Point<float> dotCentre (int i) const noexcept { return polar (ledRadius, angleAt (dotValue (i))); }

    /** The dot nearest a normalised value — the head of the lit run. */
    int dotIndexFor (float t) const noexcept
    {
        if (dots < 1) return 0;
        return juce::jlimit (0, dots - 1, (int) std::lround (juce::jlimit (0.0f, 1.0f, t) * (float) (dots - 1)));
    }
};

/** Builds the concentric geometry for a footprint (the largest centred square is used). */
inline Geometry geometry (juce::Rectangle<float> footprint, Style style, float startAngle, float endAngle) noexcept
{
    Geometry geo;
    const float d = juce::jmin (footprint.getWidth(), footprint.getHeight());
    if (d <= 0.0f) return geo;

    const auto square = footprint.withSizeKeepingCentre (d, d);
    const auto r = radii (d, style);

    geo.centre = square.getCentre();
    geo.diameter = d;
    geo.startAngle = startAngle;
    geo.endAngle = endAngle;
    geo.modStroke = r.modStroke;
    geo.modRadius = r.modRadius;
    geo.ledRadius = r.ledRadius;
    geo.dotRadius = r.dotRadius;
    geo.dots = r.dots;
    geo.capped = r.capRadius > 0.0f;
    geo.body = square.withSizeKeepingCentre (r.bodyRadius * 2.0f, r.bodyRadius * 2.0f);
    if (geo.capped) geo.cap = square.withSizeKeepingCentre (r.capRadius * 2.0f, r.capRadius * 2.0f);
    return geo;
}

//==============================================================================
namespace detail
{
    /** Stable pseudo-random in [-1, 1] for wedge `i`, so a cap's brush never crawls. */
    inline float wedgeNoise (int i) noexcept
    {
        std::uint32_t h = (std::uint32_t) i * 2654435761u;
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
        return (float) (h & 0xffffu) / 32767.5f - 1.0f;
    }

    /** Brightness 0..1 mapped onto the chassis metal ramp. */
    inline juce::Colour metalTone (float b) noexcept
    {
        b = juce::jlimit (0.0f, 1.0f, b);
        return b < 0.5f ? juce::Colour (0xff3c4049).interpolatedWith (Theme::metal, b * 2.0f)
                        : Theme::metal.interpolatedWith (Theme::metalLight, (b - 0.5f) * 2.0f);
    }
}

/** A soft round pool of light — the one soft-edged primitive everything else is built from. */
inline void softLight (juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour, float alpha)
{
    if (radius <= 0.0f || alpha <= 0.004f) return;
    juce::ColourGradient grad (colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)), centre.x, centre.y,
                               colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    grad.addColour (0.55, colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha * 0.32f)));
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
}

/**
    The shadow a round body casts on the panel it sits on.

    On a pale chassis this is the only thing that separates a knob from its
    panel, so it is offset along the light and softened, never a hard ellipse.
*/
inline void contactShadow (juce::Graphics& g, juce::Rectangle<float> circle, float spread, float strength)
{
    if (strength <= 0.01f || circle.getWidth() <= 0.0f) return;
    const float dx = -kLightX * spread * 0.55f, dy = -kLightY * spread * 0.55f;
    const auto c = circle.getCentre().translated (dx, dy);
    const float r = circle.getWidth() * 0.5f;
    juce::ColourGradient shade (juce::Colours::black.withAlpha (0.30f * strength), c.x, c.y,
                                juce::Colours::transparentBlack, c.x + r + spread, c.y, true);
    shade.addColour (juce::jlimit (0.05, 0.95, (double) (r / (r + spread))), juce::Colours::black.withAlpha (0.22f * strength));
    shade.addColour (juce::jlimit (0.06, 0.96, (double) ((r + spread * 0.45f) / (r + spread))), juce::Colours::black.withAlpha (0.07f * strength));
    g.setGradientFill (shade);
    g.fillEllipse (c.x - r - spread, c.y - r - spread, (r + spread) * 2.0f, (r + spread) * 2.0f);
}

//==============================================================================
// LED dots
//==============================================================================

/**
    An unlit dot: a hole drilled in the panel.

    Dark inside, deepest where the light cannot reach (upper left), with the
    far wall picking up a little bounce and a hairline of light on the rim
    below it. Exactly the inverse of a raised body, which is what sells "hole".
*/
inline void unlitDot (juce::Graphics& g, juce::Point<float> c, float r, float depth = 1.0f)
{
    if (r <= 0.3f) return;
    const auto hole = juce::Rectangle<float> (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    // the panel darkens very slightly around the opening
    softLight (g, c, r * 1.9f, juce::Colours::black, 0.040f * depth);

    // Inside: deepest on the wall the light cannot reach, with the far wall picking
    // up what does get in. Flat, because a hole has no form to catch a highlight.
    juce::ColourGradient inside (juce::Colour (0xff0a0c10).withAlpha (0.96f * depth),
                                 c.x + kLightX * r * 0.55f, c.y + kLightY * r * 0.55f,
                                 juce::Colour (0xff23262d).withAlpha (0.96f * depth),
                                 c.x - kLightX * r * 1.25f, c.y - kLightY * r * 1.25f, false);
    inside.addColour (0.70, juce::Colour (0xff111318).withAlpha (0.96f * depth));
    g.setGradientFill (inside);
    g.fillEllipse (hole);

    // A whisper of light on the far inner wall. Any stronger and the bore stops
    // being a hole and starts being a bead.
    juce::ColourGradient lip (juce::Colours::transparentWhite, c.x + kLightX * r, c.y + kLightY * r,
                              juce::Colours::white.withAlpha (0.17f * depth), c.x - kLightX * r, c.y - kLightY * r, false);
    lip.addColour (0.66, juce::Colours::white.withAlpha (0.015f * depth));
    g.setGradientFill (lip);
    g.drawEllipse (hole.reduced (r * 0.16f), juce::jmax (0.5f, r * 0.13f));
}

/**
    A lit dot: an amber LED with its bloom spilling onto the surface.

    `heat` is the lamp's own brightness (hover, activity and the flare of a
    freshly lit dot all push it up); `bloom` scales only the spill, so a knob
    can glow harder without the lens itself blowing out; `fade` is how far the
    lamp has come up out of its hole, which is how a dark ring warms without
    any dot ever snapping on.

    Draw it over an unlitDot: the hole is there whether or not the lamp is lit.
*/
inline void litDot (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour amber,
                    float heat, float bloom, float fade = 1.0f)
{
    if (r <= 0.3f) return;
    fade = juce::jlimit (0.0f, 1.0f, fade);
    if (fade <= 0.01f) return;
    heat = juce::jlimit (0.0f, 1.8f, heat);
    const auto lens = juce::Rectangle<float> (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    const float a = heat * fade;

    // Spill: wide and weak, then tighter and warmer. On a pale panel this has to
    // stay honest — too much alpha and the knob wears a dirty halo.
    softLight (g, c, r * (4.0f + 1.8f * bloom), amber.withRotatedHue (-0.015f), (0.13f + 0.15f * bloom) * a);
    softLight (g, c, r * (2.1f + 0.6f * bloom), amber, (0.22f + 0.18f * bloom) * a);

    // The lens itself: a hot centre falling to saturated amber at the rim.
    juce::ColourGradient lamp (amber.brighter (0.62f + 0.30f * juce::jmin (1.0f, heat)).withAlpha (juce::jmin (1.0f, 0.72f + 0.28f * heat) * fade),
                               c.x, c.y,
                               amber.darker (0.30f).withAlpha (juce::jmin (1.0f, 0.80f + 0.20f * heat) * fade),
                               c.x + r, c.y, true);
    lamp.addColour (0.45, amber.brighter (0.10f * heat).withAlpha (juce::jmin (1.0f, 0.88f + 0.12f * heat) * fade));
    g.setGradientFill (lamp);
    g.fillEllipse (lens);

    // A crisp warm rim keeps the lamp from dissolving into its own bloom.
    g.setColour (amber.darker (0.55f).withAlpha (0.55f * fade));
    g.drawEllipse (lens.reduced (r * 0.06f), juce::jmax (0.5f, r * 0.16f));

    // Specular pin on the glass, on the light's side.
    if (r > 1.6f)
    {
        const float pr = r * 0.30f;
        g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 0.9f, 0.34f + 0.34f * heat) * fade));
        g.fillEllipse (c.x + kLightX * r * 0.38f - pr, c.y + kLightY * r * 0.38f - pr, pr * 2.0f, pr * 2.0f);
    }
}

//==============================================================================
// Bodies
//==============================================================================

/**
    The moulded body of a capped knob: a matte near-black ring.

    Matte means the tone has to come from the ramp across the form, not from a
    highlight — one long gradient along the light axis, a bevel that is bright
    on the light's side and dark opposite it, and a contact shadow underneath.
*/
inline void mouldedBody (juce::Graphics& g, juce::Rectangle<float> circle, float lit, float press)
{
    const auto c = circle.getCentre();
    const float r = circle.getWidth() * 0.5f;
    if (r < 1.0f) return;

    // Pressed, the knob settles: its shadow tightens and darkens under it.
    contactShadow (g, circle, juce::jlimit (2.5f, 11.0f, r * (0.28f - 0.10f * press)), 1.25f + 0.25f * press);

    const juce::Point<float> litP (c.x + kLightX * r * 1.05f, c.y + kLightY * r * 1.05f);
    const juce::Point<float> awayP (c.x - kLightX * r * 1.15f, c.y - kLightY * r * 1.15f);

    juce::ColourGradient form (Theme::knobBase.brighter (0.40f + 0.20f * lit), litP.x, litP.y,
                               juce::Colour (0xff08090d), awayP.x, awayP.y, false);
    form.addColour (0.34, Theme::knobBase.brighter (0.06f + 0.07f * lit));
    form.addColour (0.62, Theme::knobBase.darker (0.42f));
    g.setGradientFill (form);
    g.fillEllipse (circle);

    // A broad, low sheen — moulded plastic scatters its highlight instead of
    // returning it, so this never becomes a specular dot.
    softLight (g, { c.x + kLightX * r * 0.52f, c.y + kLightY * r * 0.48f }, r * 0.95f, juce::Colours::white, 0.055f + 0.045f * lit);

    // The outer bevel: one continuous stroke, lit on the light's side, dark opposite,
    // so the edge never breaks where the two halves meet.
    {
        const float bevel = juce::jmax (0.9f, r * 0.11f);
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.24f + 0.16f * lit), c.x + kLightX * r, c.y + kLightY * r,
                                   juce::Colours::black.withAlpha (0.74f), c.x - kLightX * r, c.y - kLightY * r, false);
        edge.addColour (0.40, juce::Colours::white.withAlpha (0.025f));
        edge.addColour (0.60, juce::Colours::black.withAlpha (0.14f));
        g.setGradientFill (edge);
        g.drawEllipse (circle.reduced (bevel * 0.5f), bevel);
    }

    // A crisp silhouette against the pale panel, and a whisker of bounce beneath it.
    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.drawEllipse (circle.reduced (0.3f), juce::jmax (0.7f, r * 0.028f));
    {
        juce::ColourGradient bounce (juce::Colours::transparentWhite, c.x + kLightX * r, c.y + kLightY * r,
                                     juce::Colour (0xffb9bec8).withAlpha (0.22f), c.x - kLightX * r * 1.1f, c.y - kLightY * r * 1.1f, false);
        bounce.addColour (0.66, juce::Colours::transparentWhite);
        g.setGradientFill (bounce);
        g.drawEllipse (circle.reduced (0.6f), juce::jmax (0.6f, r * 0.022f));
    }
}

/**
    The turned metal cap.

    Narrow wedges of alternating light and shade radiate from the centre. Two
    angles drive it and they are deliberately different:

      `brushAngle`  the pattern is machined into the cap, so it turns with the
                    knob exactly;
      `sheenAngle`  where the anisotropic highlight lies. A radial brush throws
                    a two-lobed sheen, and easing this behind the value is what
                    makes the metal appear to catch the light as it is turned.

    A fixed component from the real light is mixed under both, so the cap is
    never lit from somewhere the rest of the instrument is not.
*/
inline void turnedCap (juce::Graphics& g, juce::Rectangle<float> circle, float brushAngle, float sheenAngle, float lit)
{
    const auto c = circle.getCentre();
    const float r = circle.getWidth() * 0.5f;
    if (r < 1.2f) return;

    // The seat the cap is pressed into: a dark seam, lit on the far lip.
    {
        const float seam = juce::jmax (0.8f, r * 0.13f);
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.drawEllipse (circle.expanded (seam * 0.45f), seam);
        juce::ColourGradient lip (juce::Colours::transparentWhite, c.x + kLightX * r, c.y + kLightY * r,
                                  juce::Colours::white.withAlpha (0.16f), c.x - kLightX * r, c.y - kLightY * r, false);
        lip.addColour (0.6, juce::Colours::transparentWhite);
        g.setGradientFill (lip);
        g.drawEllipse (circle.expanded (seam * 0.85f), juce::jmax (0.6f, seam * 0.5f));
    }

    juce::Graphics::ScopedSaveState save (g);
    {
        juce::Path clip;
        clip.addEllipse (circle);
        if (! g.reduceClipRegion (clip)) return;
    }

    // Base metal, so no seam between wedges can ever show the panel through.
    g.setColour (Theme::metal);
    g.fillEllipse (circle);

    // The sunburst. Wedge count follows the radius: enough that the brush stays
    // fine at 1600 x 1000 and does not turn to moire at 1100 x 690.
    // Wedges are kept wide enough at the rim to stay wedges: any narrower and the
    // alternation turns into moire long before it reads as a brush.
    const int n = juce::jlimit (28, 160, (int) std::lround (r * 2.1f));
    const float step = juce::MathConstants<float>::twoPi / (float) n;
    const float reach = r * 1.06f;
    const auto wedgeBounds = circle.withSizeKeepingCentre (reach * 2.0f, reach * 2.0f);

    for (int i = 0; i < n; ++i)
    {
        const float a0 = brushAngle + (float) i * step;
        const float theta = a0 + step * 0.5f;
        const float delta = theta - sheenAngle;

        // Two lobes: a radial brush is brightest where the grain runs across the light.
        const float lobe = 0.5f + 0.5f * std::cos (2.0f * delta);
        const float shaped = lobe * lobe * lobe * (4.0f - 3.0f * lobe);   // narrower, brighter lobes
        // Smoothed across neighbours: white noise per wedge speckles, a brush drifts.
        const float grain = 0.5f * detail::wedgeNoise (i)
                          + 0.25f * (detail::wedgeNoise (i - 1) + detail::wedgeNoise (i + 1));
        const float b = juce::jlimit (0.0f, 1.0f,
                                      0.10f + (0.80f + 0.12f * lit) * shaped
                                            + 0.145f * grain * (0.32f + 0.68f * shaped));

        juce::Path wedge;
        wedge.addPieSegment (wedgeBounds, a0, a0 + step * 1.04f, 0.0f);
        g.setColour (detail::metalTone (b));
        g.fillPath (wedge);
    }

    // The brush converges to nothing at the centre of a turned disc, so it is washed
    // out there; otherwise the wedges pile up into a starburst that is all detail
    // and no material.
    {
        juce::ColourGradient hub (Theme::metal.withAlpha (0.82f), c.x, c.y,
                                  Theme::metal.withAlpha (0.0f), c.x + r * 0.50f, c.y, true);
        hub.addColour (0.45, Theme::metal.withAlpha (0.36f));
        g.setGradientFill (hub);
        g.fillEllipse (circle);
    }

    // The fixed light, under everything the rotation does: a broad sheen in the
    // upper-left third and the bright eye a turned disc always has at its centre.
    softLight (g, { c.x + kLightX * r * 0.50f, c.y + kLightY * r * 0.46f }, r * 0.95f, juce::Colours::white, 0.15f + 0.11f * lit);
    softLight (g, c, r * 0.30f, juce::Colours::white, 0.20f + 0.10f * lit);

    // Form: the disc is very slightly domed, so it loses a little light at the rim
    // and more of it on the side facing away.
    {
        juce::ColourGradient fall (juce::Colours::transparentBlack, c.x + kLightX * r * 0.30f, c.y + kLightY * r * 0.30f,
                                   juce::Colours::black.withAlpha (0.30f), c.x - kLightX * r * 1.15f, c.y - kLightY * r * 1.15f, true);
        fall.addColour (0.58, juce::Colours::transparentBlack);
        fall.addColour (0.84, juce::Colours::black.withAlpha (0.11f));
        g.setGradientFill (fall);
        g.fillEllipse (circle);
    }

    // The crisp rim around the cap: bright where the light lands, dark opposite.
    {
        const float rimW = juce::jmax (0.7f, r * 0.075f);
        juce::ColourGradient rim (juce::Colours::white.withAlpha (0.88f), c.x + kLightX * r, c.y + kLightY * r,
                                  juce::Colour (0xff4a4f59).withAlpha (0.85f), c.x - kLightX * r, c.y - kLightY * r, false);
        rim.addColour (0.46, Theme::metalLight.withAlpha (0.55f));
        rim.addColour (0.70, Theme::metalDark.withAlpha (0.70f));
        g.setGradientFill (rim);
        g.drawEllipse (circle.reduced (rimW * 0.5f), rimW);
    }
}

/**
    Style C's body: a quiet matte dome with a concentric groove near its edge.

    No cap and no ring, so everything it says it says with tone alone — which
    means the groove and the notch have to be cut, not drawn on.
*/
inline void plainDome (juce::Graphics& g, juce::Rectangle<float> circle, float lit, float press)
{
    const auto c = circle.getCentre();
    const float r = circle.getWidth() * 0.5f;
    if (r < 1.0f) return;

    contactShadow (g, circle, juce::jlimit (2.5f, 11.0f, r * (0.26f - 0.09f * press)), 1.25f + 0.25f * press);

    const juce::Point<float> litP (c.x + kLightX * r * 0.58f, c.y + kLightY * r * 0.54f);
    const juce::Point<float> awayP (c.x - kLightX * r * 1.15f, c.y - kLightY * r * 1.12f);

    juce::ColourGradient dome (Theme::knobBase.brighter (0.62f + 0.24f * lit), litP.x, litP.y,
                               juce::Colour (0xff080910), awayP.x, awayP.y, true);
    dome.addColour (0.30, Theme::knobBase.brighter (0.26f + 0.10f * lit));
    dome.addColour (0.58, Theme::knobBase.brighter (0.0f));
    dome.addColour (0.80, Theme::knobBase.darker (0.50f));
    g.setGradientFill (dome);
    g.fillEllipse (circle);

    // Matte: one broad sheen, no core highlight. This is the knob that stays quiet.
    softLight (g, { c.x + kLightX * r * 0.46f, c.y + kLightY * r * 0.42f }, r * 0.86f, juce::Colours::white, 0.085f + 0.055f * lit);

    // The concentric groove: a dark incision with its far wall catching the light.
    {
        const float gr = r * 0.80f;
        const auto groove = circle.withSizeKeepingCentre (gr * 2.0f, gr * 2.0f);
        const float w = juce::jmax (0.7f, r * 0.045f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (groove, w);
        juce::ColourGradient wall (juce::Colours::transparentWhite, c.x + kLightX * gr, c.y + kLightY * gr,
                                   juce::Colours::white.withAlpha (0.22f + 0.10f * lit), c.x - kLightX * gr, c.y - kLightY * gr, false);
        wall.addColour (0.58, juce::Colours::transparentWhite);
        g.setGradientFill (wall);
        g.drawEllipse (groove.translated (-kLightX * w * 0.8f, -kLightY * w * 0.8f), juce::jmax (0.6f, w * 0.55f));
    }

    // Rim and silhouette.
    {
        const float bevel = juce::jmax (0.8f, r * 0.075f);
        juce::ColourGradient edge (juce::Colours::white.withAlpha (0.34f + 0.16f * lit), c.x + kLightX * r, c.y + kLightY * r,
                                   juce::Colours::black.withAlpha (0.66f), c.x - kLightX * r, c.y - kLightY * r, false);
        edge.addColour (0.42, juce::Colours::white.withAlpha (0.04f));
        g.setGradientFill (edge);
        g.drawEllipse (circle.reduced (bevel * 0.5f), bevel);
    }
    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.drawEllipse (circle.reduced (0.3f), juce::jmax (0.7f, r * 0.026f));
}

//==============================================================================
// Indicators
//==============================================================================

/**
    A cut, not a line.

    The indicator is a groove in the moulding: dark in the bottom of the cut,
    with the wall that faces the light picking up a bright edge. That is why one
    primitive can read both across a silver cap (where the dark core carries it)
    and across a near-black body (where the lit wall does).
*/
inline void indicatorCut (juce::Graphics& g, juce::Point<float> centre, float angle,
                          float innerRadius, float outerRadius, float width, float lit)
{
    if (outerRadius <= innerRadius || width <= 0.0f) return;
    const juce::Point<float> inner (centre.x + std::sin (angle) * innerRadius, centre.y - std::cos (angle) * innerRadius);
    const juce::Point<float> outer (centre.x + std::sin (angle) * outerRadius, centre.y - std::cos (angle) * outerRadius);
    const float off = width * 0.62f;

    // the wall that faces the light
    g.setColour (juce::Colours::white.withAlpha (0.40f + 0.24f * lit));
    g.drawLine (inner.x + kLightX * off, inner.y + kLightY * off, outer.x + kLightX * off, outer.y + kLightY * off, width * 0.72f);
    // the wall that does not
    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.drawLine (inner.x - kLightX * off, inner.y - kLightY * off, outer.x - kLightX * off, outer.y - kLightY * off, width * 0.72f);
    // the bottom of the cut
    g.setColour (juce::Colour (0xff0b0d12).withAlpha (0.88f));
    g.drawLine (inner.x, inner.y, outer.x, outer.y, width);
}

/**
    Style C's notch: the same cut, short, at the top of the dome.

    A plain knob has nothing else to say its value with, so the notch reaches
    almost to the rim and its lit wall is a little stronger than the indicator's
    — at the sizes this style is used for, it is the entire readout.
*/
inline void notch (juce::Graphics& g, const Geometry& geo, float angle, float lit)
{
    const float r = geo.bodyRadius();
    const float w = juce::jmax (1.2f, r * 0.095f);
    const juce::Point<float> inner (geo.centre.x + std::sin (angle) * r * 0.55f, geo.centre.y - std::cos (angle) * r * 0.55f);
    const juce::Point<float> outer (geo.centre.x + std::sin (angle) * r * 0.96f, geo.centre.y - std::cos (angle) * r * 0.96f);

    g.setColour (juce::Colours::white.withAlpha (0.54f + 0.24f * lit));
    g.drawLine (inner.x + kLightX * w * 0.6f, inner.y + kLightY * w * 0.6f,
                outer.x + kLightX * w * 0.6f, outer.y + kLightY * w * 0.6f, w * 0.68f);
    g.setColour (juce::Colour (0xff08090d).withAlpha (0.92f));
    g.drawLine (inner.x, inner.y, outer.x, outer.y, w);
}

//==============================================================================
// Modulation
//==============================================================================

/** Everything the modulation display needs, read back from the knob's ring. */
struct ModView
{
    bool  active = false;
    bool  hasRange = false;
    float base = 0.0f, lo = 0.0f, hi = 0.0f, current = 0.0f;
    int   sources = 0;
};

/**
    The modulation orbit: a second, thinner arc outside the LED ring.

    Always the modulation accent whatever the knob's section colour is, so a
    modulated control is unmistakable; the range is a band with a bracket at
    each end, and the live modulated position is a bright dot travelling along it.
*/
inline void modulationOrbit (juce::Graphics& g, const Geometry& geo, const ModView& m, juce::Colour accent, float lit)
{
    if (! m.active || geo.modRadius <= 2.0f) return;
    const float s = geo.modStroke;
    auto sweep = [&] (float from, float to)
    {
        juce::Path p;
        p.addCentredArc (geo.centre.x, geo.centre.y, geo.modRadius, geo.modRadius, 0.0f,
                         geo.angleAt (from), geo.angleAt (to), true);
        return p;
    };

    // A pale groove under the whole sweep: on a light chassis the orbit needs a
    // channel to run in or the amber has nothing to sit against.
    {
        auto full = sweep (0.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.strokePath (full, juce::PathStrokeType (s * 1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }

    if (m.hasRange)
    {
        g.setColour (accent.withAlpha (0.34f + 0.10f * lit));
        g.strokePath (sweep (m.lo, m.hi), juce::PathStrokeType (s * 1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

        // Brackets: short radial ticks that make the reach of the modulation explicit.
        for (float n : { m.lo, m.hi })
        {
            const float a = geo.angleAt (n);
            const auto p0 = geo.polar (geo.modRadius - s * 1.5f, a);
            const auto p1 = geo.polar (geo.modRadius + s * 1.5f, a);
            g.setColour (accent.withAlpha (0.80f));
            g.drawLine (p0.x, p0.y, p1.x, p1.y, juce::jmax (1.0f, s * 0.8f));
        }
    }

    // The travel from where the knob is set to where modulation has taken it.
    if (std::abs (m.current - m.base) > 0.006f)
    {
        auto travel = sweep (juce::jmin (m.base, m.current), juce::jmax (m.base, m.current));
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (travel, juce::PathStrokeType (s * 3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent.withAlpha (0.95f));
        g.strokePath (travel, juce::PathStrokeType (s * 1.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // The live position.
    {
        const auto p = geo.polar (geo.modRadius, geo.angleAt (m.current));
        const float dr = juce::jmax (1.5f, s * 1.25f);
        softLight (g, p, dr * 3.2f, accent, 0.26f + 0.14f * lit);
        g.setColour (accent.brighter (0.30f));
        g.fillEllipse (p.x - dr, p.y - dr, dr * 2.0f, dr * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (p.x - dr * 0.36f, p.y - dr * 0.36f, dr * 0.72f, dr * 0.72f);
    }
}

/** How many sources reach this destination, as a small badge on the orbit. */
inline void sourceBadge (juce::Graphics& g, const Geometry& geo, int sources, juce::Colour accent)
{
    if (sources < 2) return;
    const float r = juce::jmax (5.0f, geo.modStroke * 3.2f);
    if (geo.diameter < r * 6.0f) return;

    // Upper right, clear of the sweep's own opening at the bottom.
    const auto c = geo.polar (geo.modRadius, juce::MathConstants<float>::pi * 0.62f);
    const auto badge = juce::Rectangle<float> (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    softLight (g, c, r * 2.4f, juce::Colours::black, 0.16f);
    g.setColour (Theme::panelTop);
    g.fillEllipse (badge.expanded (juce::jmax (1.0f, r * 0.22f)));
    g.setColour (accent);
    g.fillEllipse (badge);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawEllipse (badge, juce::jmax (0.6f, r * 0.14f));

    g.setColour (juce::Colours::white);
    g.setFont (Theme::labelFontStrong (r * 1.30f));
    g.drawText (juce::String (juce::jmin (9, sources)), badge, juce::Justification::centred, false);
}

} // namespace am::ui::knobart
