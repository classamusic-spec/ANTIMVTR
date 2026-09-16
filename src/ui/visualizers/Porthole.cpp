/*
    The hardware around the ANTI-MATTER object (VISUAL_SPEC §5): the machined
    bezel, the glass composited over the object and the status lamps.

    All of it is static geometry, so the bezel, the glass highlight layer and the
    are rendered once into images at device resolution and blitted every
    frame. Only the parts that answer to the engine — the lamp bloom and the blue
    — are redrawn live.
*/

#include "AntiMatterVisualizer.h"

namespace am::ui
{

namespace
{
    constexpr float kPi    = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    inline juce::Colour alpha (juce::Colour c, float a) noexcept { return c.withAlpha (juce::jlimit (0.0f, 1.0f, a)); }
    inline juce::Point<float> polar (juce::Point<float> c, float angle, float r) noexcept
    {
        return { c.x + std::cos (angle) * r, c.y + std::sin (angle) * r };
    }

    /**
        Brushed silver: the stock the bezel is cut from (VISUAL_SPEC §4).

        It is deliberately the same metal as the turned caps on the knobs —
        `Theme::metal` and its two neighbours — so the bezel and the controls read
        as parts of one instrument. The bezel was gunmetal when the chassis was
        charcoal; against pearl a dark ring is a hole cut in the page.
    */
    const juce::Colour kMetalHi    { 0xfffbfcfd };
    const juce::Colour kMetalLight = Theme::metalLight;    // the lit face
    const juce::Colour kMetalMid   = Theme::metal;         // the body of the ring
    const juce::Colour kMetalDark  = Theme::metalDark;     // turning away from the light
    const juce::Colour kMetalDeep  { 0xff4f5560 };         // the shadowed underside
    const juce::Colour kChrome     { 0xfff8fafc };
    const juce::Colour kLamp       { 0xfffff0d2 };

    /** A ring: the outer circle with the inner one punched out (even-odd winding). */
    void makeRing (juce::Path& p, juce::Point<float> c, float outer, float inner)
    {
        p.clear();
        p.setUsingNonZeroWinding (false);
        p.addEllipse (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f);
        p.addEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f);
    }

    /**
        A bolt head seated in the silver: a shallow steel dish, shadowed under its
        upper lip and catching the light along its lower one, with a slot cut across.

        On a bright ring a bolt is a *dimple*, not a dark stud — the same reading as
        the screws on the panels, which is why it is built the same way round.
    */
    void drawBolt (juce::Graphics& g, juce::Point<float> c, float r, float lightAngle)
    {
        if (r < 1.0f) return;
        const float lx = std::cos (lightAngle), ly = std::sin (lightAngle);

        // The counterbore the head is sunk into: one fine ring, shadowed where the
        // metal steps down on the lit side and lit where it comes back up opposite.
        // A soft filled disc behind the head instead reads as a smudge on bright metal.
        {
            juce::ColourGradient bore (juce::Colours::black.withAlpha (0.30f), c.x + lx * r, c.y + ly * r,
                                       juce::Colours::white.withAlpha (0.45f), c.x - lx * r, c.y - ly * r, false);
            g.setGradientFill (bore);
            g.drawEllipse (c.x - r * 1.14f, c.y - r * 1.14f, r * 2.28f, r * 2.28f, juce::jmax (0.6f, r * 0.20f));
        }

        juce::ColourGradient body (kMetalDark, c.x + lx * r * 0.7f, c.y + ly * r * 0.7f,
                                   kMetalLight, c.x - lx * r * 1.1f, c.y - ly * r * 1.1f, false);
        body.addColour (0.5, kMetalMid);
        g.setGradientFill (body);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // A bright crescent where the light catches the far lip of the dish.
        juce::Path crescent;
        crescent.addCentredArc (c.x, c.y, r * 0.80f, r * 0.80f, 0.0f, lightAngle + kPi - 1.1f, lightAngle + kPi + 1.1f, true);
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.strokePath (crescent, juce::PathStrokeType (juce::jmax (0.6f, r * 0.22f)));

        // the slot: cut in, so it is dark with a lit lower lip
        const float s = r * 0.62f;
        g.setColour (juce::Colours::black.withAlpha (0.42f));
        g.drawLine (c.x - s, c.y - s * 0.18f, c.x + s, c.y + s * 0.18f, juce::jmax (0.7f, r * 0.20f));
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawLine (c.x - s, c.y - s * 0.18f + r * 0.22f, c.x + s, c.y + s * 0.18f + r * 0.22f, juce::jmax (0.5f, r * 0.12f));

        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, juce::jmax (0.5f, r * 0.10f));
    }
}

//==============================================================================
void AntiMatterVisualizer::refreshHardware (const Frame& f, float deviceScale)
{
    const auto& L = f.port;
    const float shadowPad = L.outerR * 0.15f;

    const auto bezelBounds = juce::Rectangle<float> (L.centreX - L.outerR - shadowPad, L.centreY - L.outerR - shadowPad,
                                                     (L.outerR + shadowPad) * 2.0f, (L.outerR + shadowPad) * 2.0f);
    const auto glassBounds = juce::Rectangle<float> (L.centreX - L.glassR, L.centreY - L.glassR, L.glassR * 2.0f, L.glassR * 2.0f);

    const auto newBezel = bezelBounds.getSmallestIntegerContainer();
    const auto newGlass = glassBounds.getSmallestIntegerContainer();

    if (std::abs (hardwareScale - deviceScale) < 0.01f
        && hardwareWidth == getWidth() && hardwareHeight == getHeight()
        && hardwareSpace == smooth.spaceType && bezelImage.isValid())
        return;

    hardwareScale = deviceScale;
    hardwareWidth = getWidth();
    hardwareHeight = getHeight();
    hardwareSpace = smooth.spaceType;

    bezelArea = newBezel; glassArea = newGlass;

    auto make = [deviceScale] (juce::Image& image, juce::Rectangle<int> area)
    {
        const int w = juce::jmax (2, juce::roundToInt ((float) area.getWidth() * deviceScale));
        const int h = juce::jmax (2, juce::roundToInt ((float) area.getHeight() * deviceScale));
        image = juce::Image (juce::Image::ARGB, w, h, true);
    };

    wellArea = newGlass;
    make (wellImage, wellArea);
    make (bezelImage, bezelArea);
    make (glassImage, glassArea);

    auto renderInto = [&] (juce::Image& image, juce::Rectangle<int> area, auto&& fn)
    {
        juce::Graphics ig (image);
        ig.addTransform (juce::AffineTransform::translation (-(float) area.getX(), -(float) area.getY())
                             .scaled (deviceScale, deviceScale));
        fn (ig);
    };

    renderInto (wellImage, wellArea, [&] (juce::Graphics& ig) { renderWell (ig, f); });
    renderInto (bezelImage, bezelArea, [&] (juce::Graphics& ig) { renderBezel (ig, f); });
    renderInto (glassImage, glassArea, [&] (juce::Graphics& ig) { renderGlass (ig, f); });
}

/** Blits a cached hardware image. A translation-only draw is JUCE's fast path. */
void AntiMatterVisualizer::blit (juce::Graphics& g, const juce::Image& image, juce::Rectangle<int> area) const
{
    if (! image.isValid()) return;
    const float inv = hardwareScale > 0.01f ? 1.0f / hardwareScale : 1.0f;
    g.drawImageTransformed (image, juce::AffineTransform::scale (inv, inv)
                                       .translated ((float) area.getX(), (float) area.getY()), false);
}

//==============================================================================
void AntiMatterVisualizer::renderBezel (juce::Graphics& g, const Frame& f)
{
    const auto& L = f.port;
    const juce::Point<float> c { L.centreX, L.centreY };
    const float outer = L.outerR;
    const float inner = L.glassR;
    const float band = L.bezelWidth;
    const float u = L.unit;

    // ---- The shadow the ring casts onto the pearl chassis: soft, below and slightly
    //      right, exactly the shadow the panels sit on. A dark halo all the way round
    //      was right against charcoal; on a pale ground it reads as a burn mark.
    draw::contactShadowEllipse (g, { c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f }, outer * 0.13f, 1.35f);

    juce::Path ring;
    makeRing (ring, c, outer, inner);

    // ---- Brushed silver: the crest catches the light along the top, the body turns
    //      away toward the bottom, and the pale chassis bounces a little back into the
    //      underside so the ring closes rather than going flat dark.
    {
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - outer * 0.35f, c.y - outer };
        gradient.point2 = { c.x + outer * 0.35f, c.y + outer };
        // The crest sits just below the top edge, not on it: the very top of a domed
        // ring is already turning away from a light that is above and to the left.
        gradient.addColour (0.0, kMetalMid.brighter (0.30f));
        gradient.addColour (0.11, kMetalHi);
        gradient.addColour (0.30, kMetalLight);
        gradient.addColour (0.52, kMetalMid);
        gradient.addColour (0.72, kMetalDark);
        gradient.addColour (0.89, kMetalDeep);
        gradient.addColour (1.0, kMetalDark.brighter (0.35f));
        g.setGradientFill (gradient);
        g.fillPath (ring);
    }

    // ---- The curve: a second, narrow highlight arc near the top edge so the ring reads as domed.
    {
        juce::Path arc;
        const float ar = outer - band * 0.26f;
        arc.addCentredArc (c.x, c.y, ar, ar, 0.0f, -kPi * 0.92f, -kPi * 0.08f, true);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - outer, c.y };
        gradient.point2 = { c.x + outer, c.y };
        gradient.addColour (0.0, juce::Colours::white.withAlpha (0.05f));
        gradient.addColour (0.34, juce::Colours::white.withAlpha (0.50f));
        gradient.addColour (0.62, juce::Colours::white.withAlpha (0.30f));
        gradient.addColour (1.0, juce::Colours::white.withAlpha (0.03f));
        g.setGradientFill (gradient);
        g.strokePath (arc, juce::PathStrokeType (band * 0.18f));
    }

    // ---- The brushing: fine turned rings round the band, alternating light and shade,
    //      which is what a machined ring looks like close up.
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (ring);
        const int streaks = juce::jlimit (8, 28, (int) (band / juce::jmax (1.0f, u * 1.2f)));
        for (int i = 0; i < streaks; ++i)
        {
            const float t = ((float) i + 0.5f) / (float) streaks;
            const float r = inner + band * t;
            const float n = noise.noise ((float) i * 3.7f, 1.4f);
            const bool light = n > 0.0f;
            g.setColour ((light ? juce::Colours::white : juce::Colours::black)
                             .withAlpha ((light ? 0.050f : 0.026f) * (0.4f + 0.6f * std::abs (n))));
            g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, juce::jmax (0.5f, u * 0.55f));
        }
    }

    // ---- Plates: the ring is bolted together from segments with fine seams between them.
    const int plates = 8;
    const float seamOffset = 0.20f;
    for (int i = 0; i < plates; ++i)
    {
        const float a = (float) i / (float) plates * kTwoPi + seamOffset;
        const auto p0 = polar (c, a, inner - u * 0.5f);
        const auto p1 = polar (c, a, outer + u * 0.5f);
        // the cut
        g.setColour (juce::Colours::black.withAlpha (0.34f));
        g.drawLine (p0.x, p0.y, p1.x, p1.y, juce::jmax (0.8f, u * 1.20f));
        // the bevel catching light on one side of the cut
        const auto q0 = polar (c, a + u * 0.010f, inner);
        const auto q1 = polar (c, a + u * 0.010f, outer);
        g.setColour (juce::Colours::white.withAlpha (0.42f));
        g.drawLine (q0.x, q0.y, q1.x, q1.y, juce::jmax (0.5f, u * 0.55f));
    }

    // ---- A bolt head at every seam, in the same style as the panel screws.
    {
        const float br = juce::jmax (1.4f, band * 0.20f);
        const float boltR = (inner + outer) * 0.5f;
        for (int i = 0; i < plates; ++i)
        {
            const float a = ((float) i + 0.5f) / (float) plates * kTwoPi + seamOffset;
            drawBolt (g, polar (c, a, boltR), br, -kPi * 0.75f);
        }
    }

    // ---- Bounce light along the underside, so the ring closes against the chassis.
    {
        juce::Path under;
        const float ur = outer - band * 0.30f;
        under.addCentredArc (c.x, c.y, ur, ur, 0.0f, kPi * 0.12f, kPi * 0.88f, true);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - outer, c.y };
        gradient.point2 = { c.x + outer, c.y };
        gradient.addColour (0.0, kChrome.withAlpha (0.05f));
        gradient.addColour (0.5, kChrome.withAlpha (0.34f));
        gradient.addColour (1.0, kChrome.withAlpha (0.05f));
        g.setGradientFill (gradient);
        g.strokePath (under, juce::PathStrokeType (band * 0.16f));
    }

    // ---- The outer edge of the ring, where the silver turns over: bright along the
    //      top where it faces the light, dark underneath where it faces the desk.
    {
        juce::Path edge;
        edge.addEllipse (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - outer * 0.5f, c.y - outer };
        gradient.point2 = { c.x + outer * 0.5f, c.y + outer };
        gradient.addColour (0.0, juce::Colours::white.withAlpha (0.85f));
        gradient.addColour (0.45, juce::Colours::white.withAlpha (0.12f));
        gradient.addColour (1.0, juce::Colours::black.withAlpha (0.34f));
        g.setGradientFill (gradient);
        g.strokePath (edge, juce::PathStrokeType (juce::jmax (1.0f, u * 1.4f)));
    }

    // ---- Machined inner edge: bright where the light hits the top, dark underneath.
    {
        juce::Path lip;
        lip.addEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - inner * 0.5f, c.y - inner };
        gradient.point2 = { c.x + inner * 0.5f, c.y + inner };
        gradient.addColour (0.0, kChrome.withAlpha (0.90f));
        gradient.addColour (0.42, kChrome.withAlpha (0.30f));
        gradient.addColour (1.0, juce::Colours::black.withAlpha (0.55f));
        g.setGradientFill (gradient);
        g.strokePath (lip, juce::PathStrokeType (juce::jmax (1.0f, u * 1.5f)));
    }

    // ---- Inner shadow onto the glass.
    {
        const float sr = inner;
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = { c.x, c.y };
        gradient.point2 = { c.x + sr, c.y };
        gradient.addColour (0.0, juce::Colours::transparentBlack);
        gradient.addColour (0.74, juce::Colours::transparentBlack);
        gradient.addColour (0.92, juce::Colours::black.withAlpha (0.34f));
        gradient.addColour (1.0, juce::Colours::black.withAlpha (0.72f));
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - sr, c.y - sr, sr * 2.0f, sr * 2.0f);
    }
}

//==============================================================================
void AntiMatterVisualizer::renderGlass (juce::Graphics& g, const Frame& f)
{
    const auto& L = f.port;
    const juce::Point<float> c { L.centreX, L.centreY };
    const float R = L.glassR;

    juce::Path disc;
    disc.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);

    juce::Graphics::ScopedSaveState clip (g);
    g.reduceClipRegion (disc);

    // ---- A broad diagonal specular sweep from the upper left. The gradient runs
    //      across the band, so the band has soft edges and no hard boundary.
    {
        const float ax = std::cos (-0.62f), ay = std::sin (-0.62f);   // sweep direction
        const float nx = -ay, ny = ax;                                 // across it
        const auto mid = juce::Point<float> (c.x - R * 0.30f, c.y - R * 0.34f);
        const float half = R * 0.46f;
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { mid.x - nx * half, mid.y - ny * half };
        gradient.point2 = { mid.x + nx * half, mid.y + ny * half };
        gradient.addColour (0.0, juce::Colours::transparentWhite);
        gradient.addColour (0.32, juce::Colours::white.withAlpha (0.055f));
        gradient.addColour (0.52, juce::Colours::white.withAlpha (0.135f));
        gradient.addColour (0.72, juce::Colours::white.withAlpha (0.045f));
        gradient.addColour (1.0, juce::Colours::transparentWhite);
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    }

    // ---- A tighter, brighter streak inside it. Filled across the whole disc rather
    //      than as a lens: a lens tapers to a point where the gradient is at full
    //      strength, and those two bright tips read as a hard edge on the glass.
    {
        const float nx = -std::sin (-0.62f), ny = std::cos (-0.62f);
        const auto mid = juce::Point<float> (c.x - R * 0.46f, c.y - R * 0.46f);
        const float half = R * 0.20f;
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { mid.x - nx * half, mid.y - ny * half };
        gradient.point2 = { mid.x + nx * half, mid.y + ny * half };
        gradient.addColour (0.0, juce::Colours::transparentWhite);
        gradient.addColour (0.30, juce::Colours::white.withAlpha (0.040f));
        gradient.addColour (0.5, juce::Colours::white.withAlpha (0.145f));
        gradient.addColour (0.70, juce::Colours::white.withAlpha (0.040f));
        gradient.addColour (1.0, juce::Colours::transparentWhite);
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    }

    // ---- The crescent hugging the inside of the bezel at the top.
    {
        juce::Path crescent;
        const float cr = R * 0.952f;
        crescent.addCentredArc (c.x, c.y, cr, cr, 0.0f, -kPi * 0.94f, -kPi * 0.06f, true);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - R, c.y };
        gradient.point2 = { c.x + R, c.y };
        gradient.addColour (0.0, juce::Colours::white.withAlpha (0.03f));
        gradient.addColour (0.28, juce::Colours::white.withAlpha (0.42f));
        gradient.addColour (0.58, juce::Colours::white.withAlpha (0.30f));
        gradient.addColour (0.84, juce::Colours::white.withAlpha (0.13f));
        gradient.addColour (1.0, juce::Colours::white.withAlpha (0.02f));
        g.setGradientFill (gradient);
        g.strokePath (crescent, juce::PathStrokeType (R * 0.058f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- Thickness: the glass darkens where you look through more of it, at the lower right.
    {
        juce::Path lens;
        lens.setUsingNonZeroWinding (false);
        lens.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
        const float ir = R * 0.845f;
        lens.addEllipse (c.x - ir - R * 0.075f, c.y - ir - R * 0.085f, ir * 2.0f, ir * 2.0f);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - R * 0.6f, c.y - R * 0.6f };
        gradient.point2 = { c.x + R * 0.75f, c.y + R * 0.75f };
        gradient.addColour (0.0, juce::Colours::transparentBlack);
        gradient.addColour (0.45, juce::Colour (0xff05060e).withAlpha (0.10f));
        gradient.addColour (1.0, juce::Colour (0xff04050c).withAlpha (0.46f));
        g.setGradientFill (gradient);
        g.fillPath (lens);
    }

    // ---- Chromatic fringe at the extreme edge: the glass splitting the light it bends.
    {
        const float w = juce::jmax (0.7f, L.unit * 0.9f);
        juce::Path a;
        a.addCentredArc (c.x, c.y, R * 0.988f, R * 0.988f, 0.0f, -kPi * 0.95f, -kPi * 0.10f, true);
        g.setColour (Theme::cyan.withAlpha (0.16f));
        g.strokePath (a, juce::PathStrokeType (w));
        juce::Path b;
        b.addCentredArc (c.x, c.y, R * 0.972f, R * 0.972f, 0.0f, kPi * 0.05f, kPi * 0.90f, true);
        g.setColour (Theme::magenta.withAlpha (0.14f));
        g.strokePath (b, juce::PathStrokeType (w));
    }
}

//==============================================================================
void AntiMatterVisualizer::drawSmudges (juce::Graphics& g, const Frame& f)
{
    // Faint, slow-moving marks so the glass reads as a real surface — never dirty.
    // Deliberately small: a radial gradient the width of the glass costs more than
    // a smudge nobody is meant to notice is worth.
    const auto& L = f.port;
    for (int i = 0; i < 2; ++i)
    {
        const float fi = (float) i;
        const float a = time * 0.013f + fi * 2.1f;
        const float d = L.glassR * (0.34f + 0.26f * noise.noise (fi * 5.0f, time * 0.02f));
        const auto p = polar ({ L.centreX, L.centreY }, a, d);
        const float r = L.glassR * (0.15f + 0.05f * fi);
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = p;
        gradient.point2 = { p.x + r, p.y };
        gradient.addColour (0.0, juce::Colours::white.withAlpha (0.026f));
        gradient.addColour (0.6, juce::Colours::white.withAlpha (0.011f));
        gradient.addColour (1.0, juce::Colours::transparentWhite);
        g.setGradientFill (gradient);
        g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
    }
}

//==============================================================================
void AntiMatterVisualizer::drawLamps (juce::Graphics& g, const Frame& f)
{
    const auto& L = f.port;
    const juce::Point<float> c { L.centreX, L.centreY };
    const float band = L.bezelWidth;
    const float mid = (L.glassR + L.outerR) * 0.5f;

    // Status: alive and breathing at rest, brighter with the level.
    const float breath = 0.5f + 0.5f * std::sin (time * 1.15f);
    const float lit = juce::jlimit (0.0f, 1.0f, 0.44f + 0.16f * breath + 0.46f * f.pulse + 0.18f * f.life);

    for (int side = 0; side < 2; ++side)
    {
        const float a = side == 0 ? kPi : 0.0f;
        const auto p = polar (c, a, mid);
        const float h = band * 1.55f, w = band * 0.30f;

        // Bloom spilling onto the metal around the lamp. Kept close to the lamp: a
        // bloom wide enough to clear the bezel lands on the pale chassis, where it is
        // not a glow but a stain — bloom is a dark-scene effect and this scene is not.
        const float br = band * (1.05f + 0.45f * lit);
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = p;
        gradient.point2 = { p.x + br, p.y };
        gradient.addColour (0.0, alpha (Theme::amber, 0.34f * lit));
        gradient.addColour (0.35, alpha (Theme::amber, 0.15f * lit));
        gradient.addColour (1.0, alpha (Theme::amber, 0.0f));
        g.setGradientFill (gradient);
        g.fillEllipse (p.x - br, p.y - br, br * 2.0f, br * 2.0f);

        // The recessed slot the bar sits in: a groove milled into the silver, with a
        // lit lip along its lower edge.
        const auto slot = juce::Rectangle<float> (p.x - w * 0.95f, p.y - h * 0.60f, w * 1.90f, h * 1.20f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillRoundedRectangle (slot.translated (0.0f, w * 0.30f), w * 0.9f);
        g.setColour (juce::Colours::black.withAlpha (0.62f));
        g.fillRoundedRectangle (slot, w * 0.9f);

        // The bar itself.
        const auto bar = juce::Rectangle<float> (p.x - w * 0.5f, p.y - h * 0.5f, w, h);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { bar.getCentreX(), bar.getY() };
        gradient.point2 = { bar.getCentreX(), bar.getBottom() };
        gradient.addColour (0.0, alpha (kLamp, 0.55f + 0.45f * lit));
        gradient.addColour (0.5, alpha (kLamp, 0.72f + 0.28f * lit));
        gradient.addColour (1.0, alpha (Theme::amber, 0.45f + 0.40f * lit));
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (bar, w * 0.5f);

        // The hot filament down the middle.
        g.setColour (juce::Colours::white.withAlpha (0.40f + 0.45f * lit));
        g.fillRoundedRectangle (bar.reduced (w * 0.32f, h * 0.10f), w * 0.2f);
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
void AntiMatterVisualizer::drawCaptions (juce::Graphics& g, const Frame& f)
{
    const auto& L = f.port;
    if (L.captionBand < 12.0f) return;

    const float h = juce::jlimit (6.5f, 10.0f, L.captionBand * 0.28f);
    const float pad = juce::jmax (4.0f, L.unit * 6.0f);
    const float w = juce::jmin (f.bounds.getWidth() * 0.30f, L.unit * 110.0f);
    auto area = f.bounds;
    const auto top = area.removeFromTop (L.captionBand);

    auto left = juce::Rectangle<float> (top.getX() + pad, top.getY() + pad * 0.4f, w, h * 1.4f);
    draw::trackedText (g, "INHALE", left, juce::Justification::topLeft, Theme::captionFont (h), alpha (Theme::textPrimary, 0.78f));
    draw::trackedText (g, "IDEA", left.translated (0.0f, h * 1.5f), juce::Justification::topLeft,
                       Theme::captionFont (h - 1.0f), Theme::textDim);

    auto right = juce::Rectangle<float> (top.getRight() - pad - w, top.getY() + pad * 0.4f, w, h * 1.4f);
    draw::trackedText (g, "EXHALE", right, juce::Justification::topRight, Theme::captionFont (h), alpha (Theme::textPrimary, 0.78f));
    draw::trackedText (g, "EVOLVE", right.translated (0.0f, h * 1.5f), juce::Justification::topRight,
                       Theme::captionFont (h - 1.0f), Theme::textDim);
}

} // namespace am::ui
