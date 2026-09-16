#include "AntiMatterVisualizer.h"
#include <cstdio>
#include <cstdlib>

namespace am::ui
{

namespace
{
    constexpr float kPi    = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
    constexpr float kFocal = 2.6f;        // camera distance in sphere radii

    inline juce::Colour alpha (juce::Colour c, float a) noexcept { return c.withAlpha (juce::jlimit (0.0f, 1.0f, a)); }
    inline float sat (float v) noexcept { return juce::jlimit (0.0f, 1.0f, v); }
    inline juce::Point<float> polar (juce::Point<float> c, float angle, float r) noexcept
    {
        return { c.x + std::cos (angle) * r, c.y + std::sin (angle) * r };
    }

    /** The radius of the dark mass, object units: MASS opens it, MELT collapses it. */
    inline float coreRadiusOf (float mass, float melt, float level) noexcept
    {
        return juce::jlimit (0.09f, 0.50f, 0.145f + 0.27f * mass - 0.05f * melt + 0.02f * level);
    }

    /** Per-quality-level caps. Level 0 = full, 1 = reduced, 2 = minimal. */
    struct QualityCaps
    {
        int   points;
        float buffer;          ///< accumulation buffer resolution as a fraction of the glass.
                               ///< Always 1: a scaled blit costs five times what the extra
                               ///< splats do, so reduced quality drops points, not pixels.
        bool  trails, wideHalo, smudges;
    };
    constexpr QualityCaps kCaps[3] = {
        { 2600, 1.00f, true,  true,  true  },
        { 2100, 1.00f, true,  false, false },
        {  900, 1.00f, false, false, false },
    };

    /** Environmental tint per Space type: NEBULA|VOID|CHAMBER|ORBIT|DREAM|MACHINE|SHIMMER|DUST */
    juce::Colour spaceTint (int type) noexcept
    {
        static const juce::Colour tints[8] = {
            Theme::violet, Theme::indigo, Theme::amber, Theme::blue,
            Theme::magenta, Theme::cyan, Theme::ivory, Theme::amber.withRotatedHue (-0.06f),
        };
        return tints[juce::jlimit (0, 7, type)];
    }
}

//==============================================================================
AntiMatterVisualizer::AntiMatterVisualizer (Diagnostics& d)
    : diag (d), noise (0x5EEDA11u), field (0xB0DE5u)
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
    profileToStderr = std::getenv ("ANTIMATR_VIS_PROFILE") != nullptr;
    if (const char* pin = std::getenv ("ANTIMATR_VIS_QUALITY"))
        quality = pinnedQuality = juce::jlimit (0, 2, std::atoi (pin));

    // Preallocated once: paint() must never allocate.
    sprites.reserve ((size_t) NodeField::kMaxPoints * 2);
    sortedSprites.resize ((size_t) NodeField::kMaxPoints * 2);
    corePath.preallocateSpace (kCoreSegs * 3 + 16);
    glassPath.preallocateSpace (32);
    gradient.clearColours();
    buildShadeTable();

    smooth.pitchHz = 220.0f;
    startTimerHz (fps);
}

AntiMatterVisualizer::~AntiMatterVisualizer() { stopTimer(); }

/**
    The object's palette, baked once: the house ramp crossed with depth into the
    volume. A point at the back of the sphere loses its colour into the cold
    distance; one at the front keeps it and gains a touch of heat. Doing this per
    point would mean a ramp evaluation and two interpolations for every one of
    thousands of splats, so it is a table read instead.
*/
void AntiMatterVisualizer::buildShadeTable()
{
    for (int d = 0; d < kDepthBins; ++d)
    {
        const float depth = ((float) d + 0.5f) / (float) kDepthBins * 2.0f - 1.0f;   // −1 far .. +1 near
        for (int h = 0; h < kHueBins; ++h)
        {
            RGB c = liquid::ramp ((float) h / (float) kHueBins);
            // Additive light desaturates wherever it piles up, so the ramp is pushed
            // away from grey before it goes into the accumulator — otherwise the
            // brightest and most interesting parts of the mass are the ones that
            // lose their colour.
            const float grey = 0.30f * c.r + 0.59f * c.g + 0.11f * c.b;
            c = { grey + (c.r - grey) * 1.34f, grey + (c.g - grey) * 1.34f, grey + (c.b - grey) * 1.34f };
            c = { liquid::clampf (c.r, 0.0f, 1.0f), liquid::clampf (c.g, 0.0f, 1.0f), liquid::clampf (c.b, 0.0f, 1.0f) };
            if (depth < 0.0f) c = liquid::deepen (c, -depth * 0.86f);
            else              c = liquid::whiten (c, depth * 0.08f);
            const size_t i = (size_t) (d * kHueBins + h) * 3;
            shade[i] = c.r; shade[i + 1] = c.g; shade[i + 2] = c.b;
        }
    }
}

void AntiMatterVisualizer::setTargetFrameRate (int newFps)
{
    fps = juce::jlimit (10, 60, newFps);
    if (isTimerRunning()) startTimerHz (fps);
}

void AntiMatterVisualizer::visibilityChanged()
{
    if (isShowing()) startTimerHz (fps); else stopTimer();
}

void AntiMatterVisualizer::resized()
{
    hardwareScale = 0.0f;               // force the cached hardware to be re-rendered
}

void AntiMatterVisualizer::parentHierarchyChanged()
{
    if (isShowing() && ! isTimerRunning()) startTimerHz (fps);
}

void AntiMatterVisualizer::timerCallback()
{
    if (! isShowing()) { stopTimer(); return; }   // restarted by parentHierarchyChanged / visibilityChanged
    const float dt = 1.0f / (float) fps;
    time += dt;
    ++frameCounter;
    latest = diag.visualSnapshots.latest();
    integrate (dt);
    updateQuality();
    repaint();
}

//==============================================================================
void AntiMatterVisualizer::integrate (float dt)
{
    // Mass = inertia: heavier matter eases more slowly toward new parameter values.
    const float rate = 7.0f - 5.6f * sat (latest.mass);
    const float ease = 1.0f - std::exp (-rate * dt);
    auto lerpTo = [ease] (float& v, float target) { v += (target - v) * ease; };
    auto easeTo = [dt] (float& v, float target, float perSecond) { v += (target - v) * (1.0f - std::exp (-perSecond * dt)); };

    lerpTo (smooth.density, latest.density); lerpTo (smooth.form, latest.form); lerpTo (smooth.mass, latest.mass);
    lerpTo (smooth.tension, latest.tension); lerpTo (smooth.decay, latest.decay); lerpTo (smooth.surface, latest.surface);
    lerpTo (smooth.bend, latest.bend); lerpTo (smooth.melt, latest.melt); lerpTo (smooth.tear, latest.tear);
    lerpTo (smooth.magnet, latest.magnet); lerpTo (smooth.gravity, latest.gravity); lerpTo (smooth.scatter, latest.scatter);
    lerpTo (smooth.crush, latest.crush);
    smooth.freeze = latest.freeze;
    smooth.fractureOn = latest.fractureOn;
    smooth.fractureActivity = latest.fractureActivity;
    easeTo (smooth.spaceActivity, latest.spaceActivity, 3.0f);
    smooth.spaceType = latest.spaceType;
    smooth.activeVoices = latest.activeVoices;
    smooth.activeNodes = latest.activeNodes;
    smooth.clusterCount = latest.clusterCount;
    smooth.rmsL = latest.rmsL; smooth.rmsR = latest.rmsR; smooth.peak = latest.peak;
    smooth.sourceRms = latest.sourceRms; smooth.matterRms = latest.matterRms;
    smooth.noteEnergy = latest.noteEnergy;
    smooth.sampleTime = latest.sampleTime;
    if (latest.pitchHz > 0.0f) smooth.pitchHz = latest.pitchHz;

    // The modal nodes. Frequencies are taken as they come — a node that moves is
    // meant to be seen moving — but energies are eased, with a fast attack and a
    // slower release, so a block boundary never strobes the swarm one lights.
    smooth.numVisualNodes = latest.numVisualNodes;
    for (int i = 0; i < latest.numVisualNodes && i < VisualStateSnapshot::kVisualNodes; ++i)
    {
        smooth.nodeFrequency[i] = latest.nodeFrequency[i];
        easeTo (smooth.nodeEnergy[i], latest.nodeEnergy[i],
                latest.nodeEnergy[i] > smooth.nodeEnergy[i] ? 26.0f : 11.0f);
        smooth.nodePan[i] = latest.nodePan[i];
        smooth.nodeCluster[i] = latest.nodeCluster[i];
    }

    // Events pass through untouched: a note id or a fracture count must arrive whole.
    smooth.noteId = latest.noteId;
    smooth.noteVelocity = latest.noteVelocity;
    smooth.noteMidi = latest.noteMidi;
    smooth.noteHeld = latest.noteHeld;
    smooth.fractureHits = latest.fractureHits;
    smooth.motionPhase = latest.motionPhase;
    smooth.motionRateHz = latest.motionRateHz;

    smooth.numVisualVoices = latest.numVisualVoices;
    for (int v = 0; v < VisualStateSnapshot::kVisualVoices; ++v)
    {
        smooth.voicePitchHz[v] = latest.voicePitchHz[v];
        easeTo (smooth.voiceEnergy[v], v < latest.numVisualVoices ? latest.voiceEnergy[v] : 0.0f, 12.0f);
        smooth.voiceVelocity[v] = latest.voiceVelocity[v];
    }

    // Fracture: only while the stage is on.
    easeTo (fracture, latest.fractureOn ? sat (latest.fractureActivity * 1.5f) : 0.0f, latest.fractureOn ? 6.0f : 2.5f);

    // Audio pulse: fast attack, release governed by Decay.
    const float level = sat (latest.rmsL * 4.5f + latest.peak * 0.5f);
    if (level > pulse) easeTo (pulse, level, 30.0f);
    else               easeTo (pulse, level, 12.0f - 10.0f * smooth.decay);

    easeTo (energy, sat (latest.noteEnergy), latest.noteEnergy > energy ? 14.0f : 4.0f - 3.0f * smooth.decay);
    easeTo (life, latest.activeVoices > 0 ? 1.0f : 0.0f, latest.activeVoices > 0 ? 4.0f : 0.8f);

    // FREEZE stops the flow dead: the flow clock and the rotation stop advancing,
    // eased over a fifth of a second so it reads as the volume seizing, not a jump cut.
    easeTo (freezeMix, latest.freeze ? 1.0f : 0.0f, 6.0f);
    const float run = 1.0f - freezeMix;

    flowTime += dt * run * (0.20f + 0.34f * smooth.decay + 0.40f * life + 0.9f * fracture);
    rotation += dt * run * (0.050f + 0.075f * (1.0f - smooth.mass)) * (0.45f + 0.55f * life) * (1.0f + 1.4f * fracture);
    if (rotation > kTwoPi) rotation -= kTwoPi;
    hueDrift += dt * run * (0.008f + 0.020f * life + 0.04f * fracture);
    if (hueDrift > 1.0f) hueDrift -= 1.0f;

    // Advance the field. It works in object units; painting converts to pixels.
    computeBudget();
    NodeField::Anim a;
    a.dt = dt; a.time = time; a.flowTime = flowTime;
    a.level = pulse; a.energy = energy; a.life = life; a.fracture = fracture;
    a.freezeMix = freezeMix; a.hueDrift = hueDrift;
    a.breathe = 1.0f + 0.020f * std::sin (time * 0.7f) * (1.0f - freezeMix)
                + 0.050f * pulse + 0.022f * energy * life;
    a.coreR = coreRadiusOf (smooth.mass, smooth.melt, pulse);

    livePoints = NodeField::pointCount (smooth, a, capPoints);
    field.update (smooth, a, noise, livePoints);
}

/**
    Detail scales with the porthole so the minimum editor size stays smooth:
    DENSITY still drives the look, but a small porthole caps how much is drawn.
    Point count follows the porthole's *area*, since that is what a splat costs.
*/
void AntiMatterVisualizer::computeBudget()
{
    const auto& caps = kCaps[juce::jlimit (0, 2, quality)];
    const auto port = PortholeLayout::forBounds (0.0f, 0.0f, (float) getWidth(), (float) getHeight());
    const float detail = juce::jlimit (0.30f, 1.0f, (port.outerR * port.outerR) / (240.0f * 240.0f));

    capPoints   = juce::jmax (96, (int) ((float) caps.points * detail));
    capBuffer   = caps.buffer;
    capTrails   = caps.trails;
    capWideHalo = caps.wideHalo;
    capSmudges  = caps.smudges;
}

void AntiMatterVisualizer::updateQuality()
{
    ++framesSinceQualityChange;
    if (frameCounter == 60) paintMsAverage = 0.0f;             // forget the cost of building the cached hardware
    if (frameCounter % 30 != 0 || frameCounter < 120) return;  // warm-up: ignore cache fills and first-frame allocations
    if (pinnedQuality >= 0) { }                                // ANTIMATR_VIS_QUALITY holds a level so it can be measured
    else if (paintMsAverage > 6.5f && quality < 2)
    {
        ++quality; framesSinceQualityChange = 0;
        recoveryWaitFrames = juce::jmin (600, recoveryWaitFrames * 2);
    }
    else if (paintMsAverage < 4.0f && quality > 0 && framesSinceQualityChange > recoveryWaitFrames)
    {
        --quality; framesSinceQualityChange = 0;
    }
    if (profileToStderr && frameCounter % 60 == 0)
    {
        static const char* names[kLayers] = { "well", "core", "splat", "resolve", "blit", "glass", "bezel", "captions" };
        std::fprintf (stderr, "[antimatter] paint %.2f ms avg, quality %d, %dx%d, points %d, texels %lld\n",
                      (double) paintMsAverage, quality, getWidth(), getHeight(), livePoints, renderer.texels());
        std::fprintf (stderr, "[antimatter] texels=%lld\n", renderer.texels());
        std::fprintf (stderr, "[antimatter] layers:");
        for (int i = 0; i < kLayers; ++i) std::fprintf (stderr, " %s=%.2f", names[i], layerMsAverage[(size_t) i]);
        std::fprintf (stderr, "\n");
    }
}

void AntiMatterVisualizer::mark (int layer) noexcept
{
    const auto now = juce::Time::getHighResolutionTicks();
    frameLayerMs[(size_t) layer] += juce::Time::highResolutionTicksToSeconds (now - lastTick) * 1000.0;
    lastTick = now;
}

//==============================================================================
void AntiMatterVisualizer::paint (juce::Graphics& g)
{
    const auto t0 = juce::Time::getHighResolutionTicks();
    lastTick = t0;
    frameLayerMs.fill (0.0);

    Frame f;
    f.bounds = getLocalBounds().toFloat();
    f.port = PortholeLayout::forBounds (f.bounds.getX(), f.bounds.getY(), f.bounds.getWidth(), f.bounds.getHeight());
    if (f.port.isTiny()) return;

    f.centre = { f.port.centreX, f.port.centreY };
    f.unit = f.port.unit;
    f.space = spaceTint (smooth.spaceType);
    f.pulse = pulse; f.energy = energy; f.life = life; f.fracture = fracture; f.hue = hueDrift;
    f.spaceActivity = smooth.spaceActivity; f.freezeMix = freezeMix;
    f.crush = sat (smooth.crush);
    f.shock = field.shockAmplitude();
    f.strike = field.strikeAmount();

    f.maxPoints = capPoints; f.bufferScale = capBuffer;
    f.trails = capTrails; f.wideHalo = capWideHalo; f.smudges = capSmudges;

    // Size and brightness follow level; the object breathes so it never looks asleep,
    // except under FREEZE, where the idle breath stops with the rest of the flow.
    f.breathe = 1.0f + 0.020f * std::sin (time * 0.7f) * (1.0f - freezeMix)
                + 0.050f * f.pulse + 0.022f * f.energy * f.life;
    f.R = f.port.glassR * (0.660f + 0.034f * smooth.mass + 0.030f * f.pulse);
    f.coreR = coreRadiusOf (smooth.mass, smooth.melt, f.pulse);

    f.cosYaw = std::cos (rotation);   f.sinYaw = std::sin (rotation);
    f.cosPitch = std::cos (0.34f);    f.sinPitch = std::sin (0.34f);

    refreshHardware (f, juce::jmax (0.5f, g.getInternalContext().getPhysicalPixelScaleFactor()));
    buildCoreOutline (f);

    // ---- 2/3/4. the well, the dark mass and the field, clipped to the glass.
    //      The field masks itself to the circle when it resolves, so a plain
    //      rectangle clip is all the glass needs and nothing can escape it.
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (juce::Rectangle<float> (f.centre.x - f.port.glassR, f.centre.y - f.port.glassR,
                                                    f.port.glassR * 2.0f, f.port.glassR * 2.0f)
                                .getSmallestIntegerContainer());
        drawWell (g, f);      mark (0);
        drawCore (g, f);      mark (1);
        drawField (g, f);     // marks 2, 3 and 4 itself
        if (f.smudges) drawSmudges (g, f);
    }

    // ---- 5. the glass, composited over the object
    blit (g, glassImage, glassArea);
    mark (5);

    // ---- 6/7. the bezel and its status lamps
    blit (g, bezelImage, bezelArea);
    drawLamps (g, f);
    mark (6);

    // ---- 8. flank captions
    drawCaptions (g, f);
    mark (7);

    const double ms = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1000.0;
    paintMsAverage = paintMsAverage <= 0.0f ? (float) ms : paintMsAverage + ((float) ms - paintMsAverage) * 0.08f;
    for (int i = 0; i < kLayers; ++i)
        layerMsAverage[(size_t) i] += (frameLayerMs[(size_t) i] - layerMsAverage[(size_t) i]) * 0.08;
}

//==============================================================================
void AntiMatterVisualizer::renderWell (juce::Graphics& g, const Frame& f)
{
    // The bowl the object is suspended in. Every colour is a Theme token shaded
    // relative to itself, so the interior travels with the chassis instead of
    // assuming one: a dark chassis gives a deep well, a pale one a frosted dish.
    // Static, so it is cached — radial gradients this large are the most expensive
    // fill in the frame and there is no budget to repeat them at 30 Hz.
    const float Rg = f.port.glassR;
    const auto c = f.centre;

    // Theme::well is the ground the object is seen against — the one token the
    // chassis reserves for behind the glass. Everything here is that colour shaded
    // relative to itself, so the interior travels with the theme instead of
    // assuming one: nothing about this bowl is a literal.
    const auto dish = Theme::well.interpolatedWith (f.space, 0.11f);
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = { c.x - Rg * 0.18f, c.y - Rg * 0.22f };
    gradient.point2 = { c.x - Rg * 0.18f + Rg * 1.30f, c.y - Rg * 0.22f };
    gradient.addColour (0.0, dish.brighter (0.09f));
    gradient.addColour (0.55, dish);
    gradient.addColour (1.0, dish.darker (0.62f));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - Rg, c.y - Rg, Rg * 2.0f, Rg * 2.0f);

    // Air in the volume: a wide, very faint environmental haze so the field has
    // something to hang in rather than floating on a flat plate.
    const float hr = Rg * 0.92f;
    const auto glow = Theme::violet.interpolatedWith (Theme::blue, 0.35f).interpolatedWith (f.space, 0.34f);
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = c;
    gradient.point2 = { c.x + hr, c.y };
    gradient.addColour (0.0, alpha (glow, 0.045f));
    gradient.addColour (0.45, alpha (glow, 0.020f));
    gradient.addColour (1.0, alpha (glow, 0.0f));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);
}

void AntiMatterVisualizer::drawWell (juce::Graphics& g, const Frame& f)
{
    // Nothing live here: the object supplies its own light now, and it does it by
    // adding energy where the points are rather than by painting a glow behind them.
    juce::ignoreUnused (f);
    blit (g, wellImage, wellArea);
}

//==============================================================================
/**
    The silhouette of the dark mass, in pixels, sampled around the circle. The
    painter strokes it and the field masks against it, so a point behind the core
    can never show through the thing it is behind.
*/
void AntiMatterVisualizer::buildCoreOutline (const Frame& f)
{
    const float cr = f.coreR * f.R * f.breathe;
    const float t = flowTime * 1.6f;
    const float mass = sat (smooth.mass), melt = sat (smooth.melt);
    const float lobes = 2.0f + 1.6f * mass;

    coreOutlineMax = 0.0f;
    for (int i = 0; i < kCoreSegs; ++i)
    {
        const float ang = (float) i / (float) kCoreSegs * kTwoPi + rotation * 0.5f;
        const float wobble = noise.ring (ang, 1.7f, t * 0.13f, 4.4f, 2) * (0.13f + 0.06f * mass);
        const float fine   = noise.ring (ang, 5.5f, t * 0.21f, 19.0f, 1) * 0.035f;
        const float sag    = melt * 0.16f * sat (std::sin (ang));
        const float lobe   = 0.05f * std::cos (lobes * ang + t * 0.17f);
        const float r = cr * juce::jlimit (0.55f, 1.55f, 1.0f + wobble + fine + lobe + sag);
        coreOutline[(size_t) i] = r;
        if (r > coreOutlineMax) coreOutlineMax = r;
    }
}

void AntiMatterVisualizer::drawCore (juce::Graphics& g, const Frame& f)
{
    const auto c = f.centre;
    const float cr = f.coreR * f.R * f.breathe;
    if (cr < 1.0f) return;

    // With nothing playing the cloud is the subject and the mass recedes: a dark
    // hole in an empty volume is the one way this object can look switched off.
    const float present = 0.46f + 0.54f * f.life;
    // The mass is the object's own matter, so it is the well's colour driven down
    // rather than a chassis token: it has to stay the darkest thing on the page
    // whichever way the theme goes.
    const auto deep  = Theme::well.darker (0.75f);
    const auto shell = Theme::well.brighter (0.10f).interpolatedWith (Theme::violet, 0.16f);

    // The volume darkening around the mass: points behind it sink into this.
    const float hr = coreOutlineMax * 1.32f;
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = c;
    gradient.point2 = { c.x + hr, c.y };
    gradient.addColour (0.0, alpha (deep, 0.62f * present));
    gradient.addColour (0.60, alpha (deep, 0.32f * present));
    gradient.addColour (1.0, alpha (deep, 0.0f));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);

    // The silhouette: a slowly deforming lobed blob, never a circle.
    corePath.clear();
    for (int i = 0; i < kCoreSegs; ++i)
    {
        const float ang = (float) i / (float) kCoreSegs * kTwoPi;
        const auto p = polar (c, ang, coreOutline[(size_t) i]);
        if (i == 0) corePath.startNewSubPath (p); else corePath.lineTo (p);
    }
    corePath.closeSubPath();

    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = { c.x - cr * 0.35f, c.y - cr * 0.40f };
    gradient.point2 = { c.x - cr * 0.35f + cr * 1.7f, c.y - cr * 0.40f };
    gradient.addColour (0.0, alpha (shell.brighter (0.30f), present));
    gradient.addColour (0.42, alpha (shell, present));
    gradient.addColour (0.78, alpha (deep.brighter (0.10f), present));
    gradient.addColour (1.0, alpha (deep, present));
    g.setGradientFill (gradient);
    g.fillPath (corePath);

    // The rim where the light wraps around the mass: a soft halo just outside the
    // silhouette, then a fine bright line on it. A strike lights it from inside.
    const float lit = sat (f.pulse + 0.55f * f.strike);
    const float rimW = juce::jmax (1.0f, f.unit * 1.7f);
    gradient.clearColours();
    gradient.isRadial = false;
    gradient.point1 = { c.x - cr, c.y - cr };
    gradient.point2 = { c.x + cr, c.y + cr };
    gradient.addColour (0.0, alpha (Theme::violet, 0.030f + 0.045f * lit));
    gradient.addColour (0.42, alpha (Theme::violet, 0.075f + 0.090f * lit));
    gradient.addColour (1.0, alpha (Theme::magenta, 0.110f + 0.100f * lit));
    g.setGradientFill (gradient);
    g.strokePath (corePath, juce::PathStrokeType (rimW * 3.0f));

    // MASS opens the core: an aperture of light widens inside the mass.
    const float open = juce::jlimit (0.0f, 1.0f, (smooth.mass - 0.42f) * 1.9f);
    if (open > 0.01f)
    {
        const float ar = cr * (0.14f + 0.44f * open);
        const auto glowCol = Theme::cyan.interpolatedWith (Theme::violet, 0.45f);
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = c;
        gradient.point2 = { c.x + ar, c.y };
        gradient.addColour (0.0, alpha (glowCol, (0.30f + 0.34f * lit) * open));
        gradient.addColour (0.55, alpha (glowCol, 0.13f * open));
        gradient.addColour (1.0, alpha (glowCol, 0.0f));
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - ar, c.y - ar, ar * 2.0f, ar * 2.0f);
    }
}

/**
    Draws the collected sprites in top-to-bottom band order.

    A counting sort into 32 horizontal bands costs two streaming passes over a
    quarter of a megabyte and buys back far more than that: within a band every
    splat writes into the same few hundred kilobytes of the accumulator, which
    stays in cache, instead of walking a megabytes-wide buffer at random.
*/
void AntiMatterVisualizer::flushSprites (float invBandHeight)
{
    const int n = (int) sprites.size();
    if (n <= 0) return;

    bandStart.fill (0);
    auto bandOf = [invBandHeight] (float y)
    {
        const int b = (int) (y * invBandHeight);
        return b < 0 ? 0 : (b >= kBands ? kBands - 1 : b);
    };
    for (int i = 0; i < n; ++i) ++bandStart[(size_t) bandOf (sprites[(size_t) i].y)];
    int total = 0;
    for (int b = 0; b < kBands; ++b) { const int c = bandStart[(size_t) b]; bandStart[(size_t) b] = total; total += c; }
    bandStart[(size_t) kBands] = total;
    for (int i = 0; i < n; ++i)
        sortedSprites[(size_t) bandStart[(size_t) bandOf (sprites[(size_t) i].y)]++] = sprites[(size_t) i];

    for (int i = 0; i < n; ++i)
    {
        const auto& s = sortedSprites[(size_t) i];
        renderer.splat (s.x, s.y, s.rad, s.r, s.g, s.b, s.lut);
    }
}

//==============================================================================
/**
    THE FIELD. Every point of the volume is projected, shaded by its depth and
    added into the accumulator; the accumulator is tone mapped once and blitted.

    Depth does four things at once, and all four together are what make the mass
    read as something with a front and a back rather than a print on the glass:
    a point at the back is smaller, dimmer, colder and softer than the same point
    at the front, and if it falls behind the silhouette of the core it is not
    drawn at all.
*/
void AntiMatterVisualizer::drawField (juce::Graphics& g, const Frame& f)
{
    // The volume is a ball inside the glass, not a wall of stars across it: it is
    // given its own radius with dark room around it, and the buffer is sized to the
    // ball rather than to the whole porthole — which is a fifth of the resolve, the
    // clear and the blit saved as well as a better read.
    const float Rg = f.port.glassR;
    const float fieldR = Rg * 0.900f;
    const float originX = std::floor (f.centre.x - fieldR);
    const float originY = std::floor (f.centre.y - fieldR);
    const float bufScale = f.bufferScale;
    const int bw = juce::jmax (8, (int) std::ceil (fieldR * 2.0f * bufScale) + 2);

    renderer.prepare (bw, bw);
    // Splat budget: room for every point to get a core and a halo, with headroom
    // for the tails. Halos and tails are dropped once it runs out, so a pathological
    // frame degrades gracefully instead of overrunning the paint budget.
    renderer.begin (juce::jmax (60000, bw * bw * 3));

    const float cxB = (f.centre.x - originX) * bufScale;
    const float cyB = (f.centre.y - originY) * bufScale;
    const float maskR = fieldR * 0.995f * bufScale;

    const float* const kCoreK = renderer.coreKernel();
    const float* const kSoftK = renderer.softKernel();
    const float* const kBodyK = f.crush > 0.35f ? renderer.hardKernel() : kCoreK;

    const float fadeIn = Rg * 0.760f, fadeOut = Rg * 0.885f;
    const float unitPx = Rg * (1.0f / 267.0f);            // one reference pixel of glass
    const float glowFloor = juce::jmax (3.5f, 8.0f * unitPx);
    const float trailGate = f.trails ? 0.0068f : 1.0e9f;   // object units moved per frame
    const int   count = field.count();

    sprites.clear();
    auto add = [this] (float x, float y, float rad, float r, float gg, float b, const float* lut)
    {
        if (sprites.size() < sprites.capacity()) sprites.push_back ({ x, y, rad, r, gg, b, lut });
    };

    for (int i = 0; i < count; ++i)
    {
        const auto& q = field.point (i);
        if (q.bright < 0.004f) continue;

        // ---- project: yaw, then the fixed pitch, then a perspective divide
        const float x1 =  q.p.x * f.cosYaw + q.p.z * f.sinYaw;
        const float z1 = -q.p.x * f.sinYaw + q.p.z * f.cosYaw;
        const float y2 =  q.p.y * f.cosPitch - z1 * f.sinPitch;
        const float z2 =  q.p.y * f.sinPitch + z1 * f.cosPitch;
        const float persp = kFocal / juce::jmax (0.7f, kFocal - z2);
        const float sx = f.centre.x + x1 * f.R * persp;
        const float sy = f.centre.y + y2 * f.R * persp;

        const float ox = sx - f.centre.x, oy = sy - f.centre.y;
        const float dist = std::sqrt (ox * ox + oy * oy);
        if (dist > fadeOut) continue;

        // The glass drinks the light at its edge, so the volume dissolves into the
        // rim instead of being cut off — and nothing can reach the bezel.
        float gain = 1.0f - liquid::smoothstep (fadeIn, fadeOut, dist);
        if (gain < 0.02f) continue;

        // ---- the mass in front of it: a point behind the core is occluded by it
        if (z2 < 0.0f && dist < coreOutlineMax * 1.55f)
        {
            int seg = (int) ((std::atan2 (oy, ox) + kPi) * ((float) kCoreSegs / kTwoPi));
            seg = seg < 0 ? 0 : (seg >= kCoreSegs ? kCoreSegs - 1 : seg);
            const float cR = coreOutline[(size_t) seg];
            gain *= liquid::smoothstep (cR * 0.98f, cR * 1.44f, dist);
            if (gain < 0.02f) continue;
        }

        const float depth01 = sat (0.5f + 0.5f * z2 / 1.25f);

        // ---- shade: the ramp crossed with depth, then pulled toward molten white
        int hb = (int) ((q.hue - std::floor (q.hue)) * (float) kHueBins);
        hb = hb < 0 ? 0 : (hb >= kHueBins ? kHueBins - 1 : hb);
        int db = (int) (depth01 * (float) kDepthBins);
        db = db < 0 ? 0 : (db >= kDepthBins ? kDepthBins - 1 : db);
        const float* const col = shade.data() + (size_t) (db * kHueBins + hb) * 3;

        // A point at the back gives up most of its light to the volume in front of it.
        const float depthGain = 0.220f + 0.780f * depth01 * depth01 * (0.34f + 0.66f * depth01);
        const float I = q.bright * 3.80f * gain * depthGain;
        const float w = q.white;
        const float cr = (col[0] + (1.0f - col[0]) * w) * I;
        const float cg = (col[1] + (1.0f - col[1]) * w) * I;
        const float cb = (col[2] + (1.0f - col[2]) * w) * I;

        // ---- size: near points are large and sharp, far ones small and soft
        const float px = f.R * q.size * persp * (0.30f + 1.25f * depth01) * (1.0f + 0.42f * q.bright);
        const float rad = juce::jlimit (0.72f * unitPx, 3.4f * unitPx, px * bufScale);
        const float bx = (sx - originX) * bufScale;
        const float by = (sy - originY) * bufScale;

        // The far half of the volume is haze and the near half is sparks. A point
        // behind the core is not just a dimmer dot — it has no hard edge at all, it
        // is a soft smear of light seen through everything in front of it. That
        // difference in *focus*, more than the difference in brightness, is what
        // gives the mass a front and a back.
        if (depth01 < 0.48f)
            add (bx, by, juce::jmin (rad * 3.0f, 6.5f * unitPx),
                 cr * 0.60f, cg * 0.60f, cb * 0.60f, kSoftK);
        else
            add (bx, by, rad, cr, cg, cb, kBodyK);

        // The light between the points. Every point also spills a wide, weak wash
        // into the glow plane, and it is those washes overlapping — not the points
        // themselves — that turn a scatter of dots into lit gas. The far half spills
        // more, because that is what being deep inside a luminous volume looks like.
        if (f.wideHalo)
        {
            // The gas does not obey the sparks' depth curve. A point at the back of a
            // luminous volume is out of focus, not extinguished: its hard light is
            // buried but its glow still reaches the eye, and that is what keeps a
            // sparse patch from reading as a handful of dots on black.
            const float Iglow = q.bright * 3.80f * gain * (0.46f + 0.54f * depthGain);
            const float gwv = Iglow * (0.150f - 0.070f * depth01);
            const float gr = (col[0] + (1.0f - col[0]) * w) * gwv;
            const float gg = (col[1] + (1.0f - col[1]) * w) * gwv;
            const float gb = (col[2] + (1.0f - col[2]) * w) * gwv;
            renderer.splatGlow (bx, by, juce::jlimit (glowFloor, 17.0f * unitPx, rad * 4.0f + 2.0f * Iglow * unitPx),
                                gr, gg, gb);
        }

        // ---- the comet tail: a point that is moving fast smears where it came from.
        //      This is what makes a shock front or a Fracture burst read as speed —
        //      far better than blurring the whole frame would.
        const float v2 = q.vel.x * q.vel.x + q.vel.y * q.vel.y + q.vel.z * q.vel.z;
        if (v2 > trailGate * trailGate)
        {
            const float ax = q.p.x - q.vel.x, ay = q.p.y - q.vel.y, az = q.p.z - q.vel.z;
            const float px1 =  ax * f.cosYaw + az * f.sinYaw;
            const float pz1 = -ax * f.sinYaw + az * f.cosYaw;
            const float py2 =  ay * f.cosPitch - pz1 * f.sinPitch;
            const float pz2 =  ay * f.sinPitch + pz1 * f.cosPitch;
            const float pp = kFocal / juce::jmax (0.7f, kFocal - pz2);
            const float tx = bx - (f.centre.x + px1 * f.R * pp - originX) * bufScale;
            const float ty = by - (f.centre.y + py2 * f.R * pp - originY) * bufScale;
            if (tx * tx + ty * ty > 1.6f)
            {
                for (int k = 1; k <= 3; ++k)
                {
                    const float t = (float) k * (1.0f / 3.0f);
                    const float a = (1.0f - t) * 0.55f;
                    add (bx - tx * t, by - ty * t, rad * (1.0f - 0.18f * (float) k),
                         cr * a, cg * a, cb * a, kCoreK);
                }
            }
        }
    }
    flushSprites ((float) kBands / (float) juce::jmax (1, bw));
    mark (2);

    renderer.resolve (cxB, cyB, maskR);
    mark (3);

    if (bufScale > 0.999f)
    {
        g.drawImageAt (renderer.image(), (int) originX, (int) originY, false);
    }
    else
    {
        g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
        g.drawImageTransformed (renderer.image(),
                                juce::AffineTransform::scale (1.0f / bufScale, 1.0f / bufScale)
                                    .translated (originX, originY), false);
    }
    mark (4);
}

} // namespace am::ui
