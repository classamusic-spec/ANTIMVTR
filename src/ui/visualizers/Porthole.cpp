/*
    The hardware around the ANTI-MATTER object (VISUAL_SPEC §5): the machined
    bezel, the glass composited over the object, the status lamps and the plinth.

    All of it is static geometry, so the bezel, the glass highlight layer and the
    plinth are rendered once into images at device resolution and blitted every
    frame. Only the parts that answer to the engine — the lamp bloom and the blue
    arc under the plinth — are redrawn live.
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

    /** Gunmetal: the bezel and plinth are cut from the same stock. */
    const juce::Colour kMetalLight { 0xff585d69 };
    const juce::Colour kMetalMid   { 0xff2c303a };
    const juce::Colour kMetalDark  { 0xff15171e };
    const juce::Colour kMetalDeep  { 0xff0a0b10 };
    const juce::Colour kChrome     { 0xffb9c2d2 };
    const juce::Colour kLamp       { 0xfffff0d2 };

    /** A ring: the outer circle with the inner one punched out (even-odd winding). */
    void makeRing (juce::Path& p, juce::Point<float> c, float outer, float inner)
    {
        p.clear();
        p.setUsingNonZeroWinding (false);
        p.addEllipse (c.x - outer, c.y - outer, outer * 2.0f, outer * 2.0f);
        p.addEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f);
    }

    /** A screw / bolt head in the panel style: dark socket, bright crescent top-left, cut slot. */
    void drawBolt (juce::Graphics& g, juce::Point<float> c, float r, float lightAngle)
    {
        if (r < 1.0f) return;
        juce::ColourGradient body (kMetalLight.darker (0.15f), c.x - r * 0.45f, c.y - r * 0.5f,
                                   juce::Colour (0xff090a0e), c.x + r * 0.6f, c.y + r * 0.7f, true);
        g.setGradientFill (body);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // socket shadow around the head
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, juce::jmax (0.6f, r * 0.16f));

        // bright crescent on the lit side
        juce::Path crescent;
        crescent.addCentredArc (c.x, c.y, r * 0.78f, r * 0.78f, 0.0f, lightAngle - 1.15f, lightAngle + 1.15f, true);
        g.setColour (juce::Colours::white.withAlpha (0.38f));
        g.strokePath (crescent, juce::PathStrokeType (juce::jmax (0.6f, r * 0.24f)));

        // the slot
        const float s = r * 0.62f;
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawLine (c.x - s, c.y - s * 0.18f, c.x + s, c.y + s * 0.18f, juce::jmax (0.7f, r * 0.20f));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (c.x - s, c.y - s * 0.18f + r * 0.22f, c.x + s, c.y + s * 0.18f + r * 0.22f, juce::jmax (0.5f, r * 0.12f));
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
    const auto plinthBounds = juce::Rectangle<float> (L.centreX - L.plinthHalfWidth * 1.20f, L.plinthTop - L.plinthHeight * 0.30f,
                                                      L.plinthHalfWidth * 2.40f, L.plinthHeight * 1.60f);

    const auto newBezel = bezelBounds.getSmallestIntegerContainer();
    const auto newGlass = glassBounds.getSmallestIntegerContainer();
    const auto newPlinth = plinthBounds.getSmallestIntegerContainer();

    if (std::abs (hardwareScale - deviceScale) < 0.01f
        && hardwareWidth == getWidth() && hardwareHeight == getHeight()
        && hardwareSpace == smooth.spaceType && bezelImage.isValid())
        return;

    hardwareScale = deviceScale;
    hardwareWidth = getWidth();
    hardwareHeight = getHeight();
    hardwareSpace = smooth.spaceType;

    bezelArea = newBezel; glassArea = newGlass; plinthArea = newPlinth;

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
    make (plinthImage, plinthArea);

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
    renderInto (plinthImage, plinthArea, [&] (juce::Graphics& ig) { renderPlinth (ig, f); });
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

    // ---- Outer shadow onto the panel behind the bezel.
    {
        const float sr = outer * 1.20f;
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = { c.x, c.y + u * 2.0f };
        gradient.point2 = { c.x + sr, c.y + u * 2.0f };
        gradient.addColour (0.0, juce::Colours::transparentBlack);
        gradient.addColour (0.76, juce::Colours::transparentBlack);
        gradient.addColour (0.845, juce::Colours::black.withAlpha (0.62f));
        gradient.addColour (1.0, juce::Colours::transparentBlack);
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - sr, c.y - sr + u * 2.0f, sr * 2.0f, sr * 2.0f);
    }

    juce::Path ring;
    makeRing (ring, c, outer, inner);

    // ---- Brushed gunmetal: light at the top, dark at the bottom, with a lift where
    //      the panel bounces light back into the underside.
    {
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x, c.y - outer };
        gradient.point2 = { c.x, c.y + outer };
        gradient.addColour (0.0, kMetalLight.brighter (0.12f));
        gradient.addColour (0.14, kMetalLight);
        gradient.addColour (0.44, kMetalMid);
        gradient.addColour (0.70, kMetalDark.brighter (0.22f));
        gradient.addColour (0.88, kMetalDark);
        gradient.addColour (1.0, kMetalMid.brighter (0.08f));
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
        gradient.addColour (0.0, juce::Colours::white.withAlpha (0.02f));
        gradient.addColour (0.38, juce::Colours::white.withAlpha (0.34f));
        gradient.addColour (0.62, juce::Colours::white.withAlpha (0.30f));
        gradient.addColour (1.0, juce::Colours::white.withAlpha (0.02f));
        g.setGradientFill (gradient);
        g.strokePath (arc, juce::PathStrokeType (band * 0.20f));
    }

    // ---- Fine brushed streaks running around the ring.
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (ring);
        const int streaks = juce::jlimit (6, 22, (int) (band / juce::jmax (1.0f, u * 1.6f)));
        for (int i = 0; i < streaks; ++i)
        {
            const float t = ((float) i + 0.5f) / (float) streaks;
            const float r = inner + band * t;
            const float n = noise.noise ((float) i * 3.7f, 1.4f);
            g.setColour (juce::Colours::white.withAlpha (0.012f + 0.020f * (0.5f + 0.5f * n)));
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
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.drawLine (p0.x, p0.y, p1.x, p1.y, juce::jmax (0.8f, u * 1.35f));
        // the bevel catching light on one side of the cut
        const auto q0 = polar (c, a + u * 0.010f, inner);
        const auto q1 = polar (c, a + u * 0.010f, outer);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine (q0.x, q0.y, q1.x, q1.y, juce::jmax (0.5f, u * 0.55f));
    }

    // ---- A bolt head at every seam, in the same style as the panel screws.
    {
        const float br = juce::jmax (1.4f, band * 0.27f);
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
        gradient.addColour (0.0, kChrome.withAlpha (0.03f));
        gradient.addColour (0.5, kChrome.withAlpha (0.20f));
        gradient.addColour (1.0, kChrome.withAlpha (0.03f));
        g.setGradientFill (gradient);
        g.strokePath (under, juce::PathStrokeType (band * 0.16f));
    }

    // ---- Machined inner edge: bright where the light hits the top, dark underneath.
    {
        juce::Path lip;
        lip.addEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - inner * 0.5f, c.y - inner };
        gradient.point2 = { c.x + inner * 0.5f, c.y + inner };
        gradient.addColour (0.0, kChrome.withAlpha (0.72f));
        gradient.addColour (0.42, kChrome.withAlpha (0.16f));
        gradient.addColour (1.0, juce::Colours::black.withAlpha (0.60f));
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
        gradient.addColour (0.34, juce::Colours::white.withAlpha (0.035f));
        gradient.addColour (0.52, juce::Colours::white.withAlpha (0.085f));
        gradient.addColour (0.70, juce::Colours::white.withAlpha (0.028f));
        gradient.addColour (1.0, juce::Colours::transparentWhite);
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    }

    // ---- A tighter, brighter streak inside it.
    {
        const float ax = std::cos (-0.62f), ay = std::sin (-0.62f);
        const float nx = -ay, ny = ax;
        const auto mid = juce::Point<float> (c.x - R * 0.46f, c.y - R * 0.46f);
        const float half = R * 0.17f;
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { mid.x - nx * half, mid.y - ny * half };
        gradient.point2 = { mid.x + nx * half, mid.y + ny * half };
        gradient.addColour (0.0, juce::Colours::transparentWhite);
        gradient.addColour (0.5, juce::Colours::white.withAlpha (0.075f));
        gradient.addColour (1.0, juce::Colours::transparentWhite);
        g.setGradientFill (gradient);
        // A lens-shaped sweep rather than a full band: taper it toward the ends.
        juce::Path sweep;
        const auto a0 = juce::Point<float> (mid.x - ax * R * 0.92f, mid.y - ay * R * 0.92f);
        const auto a1 = juce::Point<float> (mid.x + ax * R * 0.92f, mid.y + ay * R * 0.92f);
        sweep.startNewSubPath (a0);
        sweep.quadraticTo (mid.x + nx * half, mid.y + ny * half, a1.x, a1.y);
        sweep.quadraticTo (mid.x - nx * half, mid.y - ny * half, a0.x, a0.y);
        sweep.closeSubPath();
        g.fillPath (sweep);
    }

    // ---- The crescent hugging the inside of the bezel at the top.
    {
        juce::Path crescent;
        const float cr = R * 0.955f;
        crescent.addCentredArc (c.x, c.y, cr, cr, 0.0f, -kPi * 0.86f, -kPi * 0.12f, true);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { c.x - R, c.y };
        gradient.point2 = { c.x + R, c.y };
        gradient.addColour (0.0, juce::Colours::transparentWhite);
        gradient.addColour (0.30, juce::Colours::white.withAlpha (0.26f));
        gradient.addColour (0.56, juce::Colours::white.withAlpha (0.16f));
        gradient.addColour (1.0, juce::Colours::transparentWhite);
        g.setGradientFill (gradient);
        g.strokePath (crescent, juce::PathStrokeType (R * 0.045f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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

        // Bloom spilling onto the metal around the lamp.
        const float br = band * (1.9f + 0.7f * lit);
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = p;
        gradient.point2 = { p.x + br, p.y };
        gradient.addColour (0.0, alpha (Theme::amber, 0.30f * lit));
        gradient.addColour (0.35, alpha (Theme::amber, 0.14f * lit));
        gradient.addColour (1.0, alpha (Theme::amber, 0.0f));
        g.setGradientFill (gradient);
        g.fillEllipse (p.x - br, p.y - br, br * 2.0f, br * 2.0f);

        // The recessed slot the bar sits in.
        const auto slot = juce::Rectangle<float> (p.x - w * 0.95f, p.y - h * 0.60f, w * 1.90f, h * 1.20f);
        g.setColour (juce::Colours::black.withAlpha (0.75f));
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
void AntiMatterVisualizer::drawPlinthGlow (juce::Graphics& g, const Frame& f)
{
    const auto& L = f.port;
    if (L.plinthHeight < 6.0f) return;

    // A blue arc spilling from beneath the plinth onto the panel, brightest
    // directly under the sphere and answering the level.
    const juce::Point<float> p { L.centreX, L.plinthTop + L.plinthHeight * 0.58f };
    const float rx = L.plinthHalfWidth * 1.18f;
    const float ry = juce::jmax (4.0f, L.plinthHeight * 0.92f);
    const float a = 0.22f + 0.30f * f.pulse + 0.12f * f.life + 0.06f * f.energy;
    const auto col = Theme::blue.interpolatedWith (Theme::cyan, 0.22f + 0.25f * f.pulse);

    juce::Graphics::ScopedSaveState state (g);
    // Only the part below the stand is ever seen; the plinth covers the rest.
    g.reduceClipRegion (juce::Rectangle<float> (f.bounds.getX(), L.plinthTop - L.plinthHeight * 0.12f,
                                                f.bounds.getWidth(), f.bounds.getBottom() - L.plinthTop + L.plinthHeight)
                            .getSmallestIntegerContainer());
    g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, p.x, p.y));
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = p;
    gradient.point2 = { p.x + rx, p.y };
    gradient.addColour (0.0, alpha (col, a));
    gradient.addColour (0.32, alpha (col, a * 0.55f));
    gradient.addColour (0.66, alpha (col, a * 0.18f));
    gradient.addColour (1.0, alpha (col, 0.0f));
    g.setGradientFill (gradient);
    g.fillEllipse (p.x - rx, p.y - rx, rx * 2.0f, rx * 2.0f);
}

//==============================================================================
void AntiMatterVisualizer::renderPlinth (juce::Graphics& g, const Frame& f)
{
    const auto& L = f.port;
    if (L.plinthHeight < 6.0f) return;

    const float cx = L.centreX;
    const float top = L.plinthTop;
    const float rx = L.plinthHalfWidth;
    const float ry = juce::jmax (2.0f, L.plinthHeight * 0.26f);
    const float bodyH = L.plinthHeight * 0.62f;
    const float u = juce::jmax (0.6f, L.unit);

    // ---- Contact shadow under the whole stand.
    {
        const float sr = rx * 1.05f;
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (1.0f, (bodyH * 0.55f) / sr, cx, top + bodyH));
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = { cx, top + bodyH };
        gradient.point2 = { cx + sr, top + bodyH };
        gradient.addColour (0.0, juce::Colours::black.withAlpha (0.65f));
        gradient.addColour (0.62, juce::Colours::black.withAlpha (0.30f));
        gradient.addColour (1.0, juce::Colours::transparentBlack);
        g.setGradientFill (gradient);
        g.fillEllipse (cx - sr, top + bodyH - sr, sr * 2.0f, sr * 2.0f);
    }

    // ---- The body: the front face of the machined stand, tapering slightly inward.
    juce::Path face;
    {
        const float bx = rx * 0.90f;
        face.startNewSubPath (cx - rx, top);
        face.lineTo (cx - bx, top + bodyH * 0.82f);
        face.quadraticTo (cx - bx * 0.55f, top + bodyH + ry * 0.55f, cx, top + bodyH + ry * 0.62f);
        face.quadraticTo (cx + bx * 0.55f, top + bodyH + ry * 0.55f, cx + bx, top + bodyH * 0.82f);
        face.lineTo (cx + rx, top);
        face.closeSubPath();

        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { cx, top };
        gradient.point2 = { cx, top + bodyH + ry };
        gradient.addColour (0.0, kMetalMid.brighter (0.28f));
        gradient.addColour (0.24, kMetalMid);
        gradient.addColour (0.70, kMetalDark);
        gradient.addColour (1.0, juce::Colour (0xff1e222a));
        g.setGradientFill (gradient);
        g.fillPath (face);

        // Brushed streaks across the face.
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (face);
        const int streaks = juce::jlimit (4, 14, (int) (bodyH / juce::jmax (1.0f, u * 2.4f)));
        for (int i = 0; i < streaks; ++i)
        {
            const float y = top + bodyH * ((float) i + 0.5f) / (float) streaks;
            const float n = noise.noise ((float) i * 4.1f, 7.7f);
            g.setColour (juce::Colours::white.withAlpha (0.010f + 0.016f * (0.5f + 0.5f * n)));
            g.drawLine (cx - rx, y, cx + rx, y, juce::jmax (0.5f, u * 0.6f));
        }
    }

    // ---- The top surface: an ellipse seen from slightly above, with a chrome rim.
    {
        const auto plate = juce::Rectangle<float> (cx - rx, top - ry, rx * 2.0f, ry * 2.0f);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { cx, plate.getY() };
        gradient.point2 = { cx, plate.getBottom() };
        gradient.addColour (0.0, kMetalDark);
        gradient.addColour (0.42, kMetalMid.brighter (0.16f));
        gradient.addColour (1.0, kMetalDark.darker (0.25f));
        g.setGradientFill (gradient);
        g.fillEllipse (plate);

        // Chrome rim catching the light along the top edge.
        juce::Path lip;
        lip.addCentredArc (cx, top, rx - u * 0.5f, ry - u * 0.5f, 0.0f, -kPi * 0.98f, -kPi * 0.02f, true);
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { cx - rx, top };
        gradient.point2 = { cx + rx, top };
        gradient.addColour (0.0, kChrome.withAlpha (0.18f));
        gradient.addColour (0.34, kChrome.withAlpha (0.92f));
        gradient.addColour (0.64, kChrome.withAlpha (0.55f));
        gradient.addColour (1.0, kChrome.withAlpha (0.16f));
        g.setGradientFill (gradient);
        g.strokePath (lip, juce::PathStrokeType (juce::jmax (1.0f, u * 1.7f)));

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        juce::Path under;
        under.addCentredArc (cx, top, rx - u * 0.5f, ry - u * 0.5f, 0.0f, kPi * 0.04f, kPi * 0.96f, true);
        g.strokePath (under, juce::PathStrokeType (juce::jmax (0.7f, u * 1.0f)));
    }

    // ---- The recessed face carrying the engraved wordmark.
    {
        const float insetW = rx * 1.22f;
        const float insetH = bodyH * 0.80f;
        const auto inset = juce::Rectangle<float> (cx - insetW * 0.5f, top + bodyH * 0.09f, insetW, insetH);
        const float corner = insetH * 0.22f;

        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = { cx, inset.getY() };
        gradient.point2 = { cx, inset.getBottom() };
        gradient.addColour (0.0, juce::Colour (0xff101219));
        gradient.addColour (1.0, juce::Colour (0xff262a33));
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (inset, corner);

        g.setColour (juce::Colours::black.withAlpha (0.60f));
        g.drawRoundedRectangle (inset.reduced (0.5f), corner, juce::jmax (0.7f, u * 0.8f));
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.drawLine (inset.getX() + corner, inset.getBottom() - u * 0.5f, inset.getRight() - corner, inset.getBottom() - u * 0.5f,
                    juce::jmax (0.6f, u * 0.7f));

        // Engraved: cut in, so the letters are dark with a light lower edge.
        const float titleH = juce::jlimit (7.0f, 24.0f, insetH * 0.36f);
        const float subH = juce::jlimit (5.0f, 11.0f, insetH * 0.17f);
        auto textArea = inset.reduced (insetW * 0.05f, insetH * 0.10f);
        auto titleRow = textArea.removeFromTop (titleH * 1.20f);
        const float lift = juce::jmax (0.7f, u * 0.9f);

        auto title = draw::fitFont (Theme::displayFont (titleH, 0.20f), "ANTI-MATR", titleRow.getWidth(), 7.0f);
        draw::trackedText (g, "ANTI-MATR", titleRow.translated (0.0f, lift), juce::Justification::centred, title,
                           kChrome.withAlpha (0.55f));
        draw::trackedText (g, "ANTI-MATR", titleRow, juce::Justification::centred, title, juce::Colour (0xff05060a).withAlpha (0.92f));

        if (textArea.getHeight() > subH * 1.0f)
        {
            auto subRow = textArea.removeFromTop (juce::jmin (textArea.getHeight(), subH * 1.7f));
            auto sub = draw::fitFont (Theme::captionFont (subH), "SOUND BEYOND MATTER", subRow.getWidth(), 5.0f);
            draw::trackedText (g, "SOUND BEYOND MATTER", subRow.translated (0.0f, lift * 0.7f), juce::Justification::centred, sub,
                               kChrome.withAlpha (0.38f));
            draw::trackedText (g, "SOUND BEYOND MATTER", subRow, juce::Justification::centred, sub,
                               juce::Colour (0xff06070c).withAlpha (0.88f));
        }
    }

    // ---- Two bolts holding the stand to the chassis.
    {
        const float br = juce::jmax (1.2f, u * 3.0f);
        drawBolt (g, { cx - rx * 0.80f, top + bodyH * 0.42f }, br, -kPi * 0.75f);
        drawBolt (g, { cx + rx * 0.80f, top + bodyH * 0.42f }, br, -kPi * 0.75f);
    }
}

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
