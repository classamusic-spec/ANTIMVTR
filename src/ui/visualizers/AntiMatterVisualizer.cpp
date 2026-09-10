#include "AntiMatterVisualizer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace am::ui
{

namespace
{
    constexpr float kPi    = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
    constexpr float kBackdropRes = 0.5f;   // soft layers are rendered at half resolution and upscaled

    inline juce::Colour alpha (juce::Colour c, float a) noexcept { return c.withAlpha (juce::jlimit (0.0f, 1.0f, a)); }
    inline float sat (float v) noexcept { return juce::jlimit (0.0f, 1.0f, v); }
    inline float smoothstep (float a, float b, float x) noexcept { return ObjectField::smoothstep (a, b, x); }
    inline juce::Point<float> polar (juce::Point<float> c, float angle, float r) noexcept { return { c.x + std::cos (angle) * r, c.y + std::sin (angle) * r }; }
    inline float hash01 (int i) noexcept { const float v = std::sin ((float) i * 12.9898f + 78.233f) * 43758.5453f; return v - std::floor (v); }

    // Per-quality-level caps. Level 0 = full, 1 = reduced, 2 = minimal.
    struct QualityCaps { int rimSegments, shells, filaments, fragments, sparks, backdropInterval; bool streaks, filamentHalo, fragmentGlow; };
    constexpr QualityCaps kCaps[3] = {
        { 128, 3, 14, 48, 96, 3, true,  true,  true  },
        {  80, 2, 10, 32, 48, 4, true,  false, false },
        {  48, 1,  6, 20,  0, 6, false, false, false },
    };

    // Environmental halo tint per Space type: NEBULA|VOID|CHAMBER|ORBIT|DREAM|MACHINE|SHIMMER|DUST
    struct SpaceLook { juce::Colour tint; float strength; };
    const SpaceLook& spaceLook (int type)
    {
        static const SpaceLook looks[8] = {
            { Theme::violet,               1.00f },
            { juce::Colour (0xff23233d),   0.55f },
            { Theme::amber,                0.55f },
            { Theme::blue,                 0.95f },
            { Theme::magenta,              0.80f },
            { Theme::cyan,                 0.75f },
            { Theme::ivory,                0.70f },
            { juce::Colour (0xffa8957f),   0.60f },
        };
        return looks[juce::jlimit (0, 7, type)];
    }

    juce::Colour clusterColour (int cluster)
    {
        static const juce::Colour colours[6] = { Theme::cyan, Theme::magenta, Theme::violet, Theme::ivory, Theme::blue, Theme::amber };
        return colours[cluster % 6];
    }
}

/** Everything derived once per paint from the eased state and the component bounds. */
struct AntiMatterVisualizer::Frame
{
    juce::Rectangle<float> bounds, square;
    juce::Point<float> centre;           // object centre (between the lobes when torn)
    float side = 1.0f;                   // square side
    float px = 1.0f;                     // pixel scale relative to the 486 px reference
    float R = 1.0f;                      // base radius of the object
    float coreR = 1.0f;                  // void core radius
    float lobeAlpha[2] { 1.0f, 0.0f };
    float lobeScale[2] { 1.0f, 1.0f };
    juce::Point<float> lobeCentre[2];
    int   numLobes = 1;
    int   numShells = 2;
    int   numFilaments = 8;
    int   numFragments = 24;
    int   numSparks = 40;
    float tearAxis = 0.0f;
    QualityCaps caps {};
    SpaceLook space {};
    // eased state shortcuts
    float density, form, mass, tension, decay, surface, bend, melt, tear, magnet, gravity, scatter, crush;
    float pulse, energy, life, fracture, spaceActivity, pitchU, hue, rotation, time;
};

//==============================================================================
AntiMatterVisualizer::AntiMatterVisualizer (Diagnostics& d)
    : diag (d), noise (0x5EEDA11u), particles (0xA11CEu)
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
    profileToStderr = std::getenv ("ANTIMATR_VIS_PROFILE") != nullptr;

    bodyPath.preallocateSpace (ObjectField::kSamples * 3 + 8);
    shellPath.preallocateSpace (ObjectField::kSamples * 3 + 8);
    quad.preallocateSpace (24);
    curve.preallocateSpace (64);
    glyph.preallocateSpace (64);
    ringPath.preallocateSpace (64);

    smooth.pitchHz = 220.0f;
    startTimerHz (fps);
}

AntiMatterVisualizer::~AntiMatterVisualizer() { stopTimer(); }

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
    backdropAge = 1000;   // force a re-render at the new size
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
    smooth.numVisualNodes = latest.numVisualNodes;
    for (int i = 0; i < latest.numVisualNodes && i < VisualStateSnapshot::kVisualNodes; ++i)
    {
        smooth.nodeFrequency[i] = latest.nodeFrequency[i];
        easeTo (smooth.nodeEnergy[i], latest.nodeEnergy[i], 18.0f);
        smooth.nodePan[i] = latest.nodePan[i];
        smooth.nodeCluster[i] = latest.nodeCluster[i];
    }

    // Fracture: shards only when the stage is on; eased so bursts read as motion, not flicker.
    easeTo (fracture, latest.fractureOn ? sat (latest.fractureActivity * 1.5f) : 0.0f, latest.fractureOn ? 6.0f : 2.5f);

    // Audio pulse: fast attack, release governed by Decay.
    const float level = sat (latest.rmsL * 4.5f + latest.peak * 0.5f);
    if (level > pulse) easeTo (pulse, level, 30.0f);
    else               easeTo (pulse, level, 12.0f - 10.0f * smooth.decay);

    // Note energy and "life" (idle → slow breathing, never dead).
    easeTo (energy, sat (latest.noteEnergy), latest.noteEnergy > energy ? 14.0f : 4.0f - 3.0f * smooth.decay);
    easeTo (life, latest.activeVoices > 0 ? 1.0f : 0.0f, latest.activeVoices > 0 ? 4.0f : 0.8f);

    // Pitch → structure (log scale 30 Hz .. ~15 kHz)
    const float pu = sat (std::log2 (juce::jmax (30.0f, smooth.pitchHz) / 30.0f) / 9.0f);
    easeTo (pitchU, pu, 5.0f);

    // Slow structural rotation: lighter and more resonant (Decay) matter turns faster; fracture spins it up.
    rotation += dt * (0.05f + 0.08f * (1.0f - smooth.mass)) * (0.35f + 0.65f * smooth.decay) * (0.45f + 0.55f * life) * (1.0f + 2.0f * fracture);
    if (rotation > kTwoPi) rotation -= kTwoPi;
    hueDrift += dt * (0.012f + 0.03f * life + 0.06f * fracture);
    if (hueDrift > 1.0f) hueDrift -= 1.0f;

    // Particles advance in object units; drawing converts to pixels.
    const auto& caps = kCaps[quality];
    ParticleSystem::Env env;
    env.dt = dt; env.time = time; env.decay = smooth.decay; env.magnet = smooth.magnet; env.scatter = smooth.scatter;
    env.tear = smooth.tear; env.gravity = smooth.gravity; env.density = smooth.density; env.fracture = fracture;
    env.life = life; env.pulse = pulse; env.tension = smooth.tension; env.melt = smooth.melt;
    const int numFragments = juce::jlimit (8, caps.fragments, 10 + (int) (smooth.density * 26.0f) + (int) (fracture * 20.0f));
    const int numSparks = juce::jmin (caps.sparks, 14 + (int) (smooth.density * 50.0f * (0.35f + 0.65f * life)) + (int) (fracture * 30.0f));
    particles.updateFragments (env, noise, numFragments);
    particles.updateSparks (env, noise, numSparks);
}

void AntiMatterVisualizer::updateQuality()
{
    ++framesSinceQualityChange;
    if (frameCounter % 30 != 0) return;
    if (paintMsAverage > 8.0f && quality < 2)
    {
        ++quality; framesSinceQualityChange = 0; backdropAge = 1000;
    }
    else if (paintMsAverage < 3.5f && quality > 0 && framesSinceQualityChange > 240)
    {
        --quality; framesSinceQualityChange = 0; backdropAge = 1000;
    }
    if (profileToStderr && frameCounter % 60 == 0)
    {
        std::fprintf (stderr, "[antimatter] paint %.2f ms avg, quality %d, %dx%d, fps %d\n",
                      (double) paintMsAverage, quality, getWidth(), getHeight(), fps);
        static const char* names[kLayers] = { "backdrop", "stars", "farFrag", "strands", "geometry", "volume", "shells", "filaments",
                                              "core", "highlight", "rim", "cracks", "sparks", "nearFrag", "nodes", "captions", "blit" };
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
    f.side = juce::jmin (f.bounds.getWidth(), f.bounds.getHeight());
    if (f.side < 40.0f) { drawCaptions (g, f.bounds); return; }
    f.square = f.bounds.withSizeKeepingCentre (f.side, f.side);
    f.centre = f.square.getCentre();
    f.px = f.side / 486.0f;
    f.caps = kCaps[quality];
    f.space = spaceLook (smooth.spaceType);

    f.density = smooth.density; f.form = smooth.form; f.mass = smooth.mass; f.tension = smooth.tension; f.decay = smooth.decay;
    f.surface = smooth.surface; f.bend = smooth.bend; f.melt = smooth.melt; f.tear = smooth.tear; f.magnet = smooth.magnet;
    f.gravity = smooth.gravity; f.scatter = smooth.scatter; f.crush = smooth.crush;
    f.pulse = pulse; f.energy = energy; f.life = life; f.fracture = fracture; f.spaceActivity = smooth.spaceActivity;
    f.pitchU = pitchU; f.hue = hueDrift; f.rotation = rotation; f.time = time;

    // Size: heavier matter is a little larger; breathing keeps the object alive when idle; audio pulses it.
    const float breathe = 1.0f + 0.016f * std::sin (time * 0.75f) + 0.045f * pulse + 0.02f * energy * life;
    f.R = f.side * (0.29f + 0.03f * f.mass);
    f.coreR = f.R * (0.26f + 0.11f * f.mass - 0.06f * f.pitchU) * (1.0f - 0.25f * f.melt) * (0.94f + 0.12f * f.gravity);

    f.numShells = juce::jlimit (1, f.caps.shells, 1 + (int) std::lround (f.density * 2.4f));
    f.numFilaments = juce::jlimit (3, f.caps.filaments, 4 + (int) (f.density * 10.0f));
    f.numFragments = juce::jlimit (8, f.caps.fragments, 10 + (int) (f.density * 26.0f) + (int) (f.fracture * 20.0f));
    f.numSparks = juce::jmin (f.caps.sparks, 14 + (int) (f.density * 50.0f * (0.35f + 0.65f * f.life)) + (int) (f.fracture * 30.0f));

    // Tear: the object separates into two lobes along a slowly turning axis.
    f.tearAxis = 0.55f + 0.3f * std::sin (time * 0.05f);
    const float tearShown = smoothstep (0.02f, 0.45f, f.tear);
    f.numLobes = f.tear > 0.02f ? 2 : 1;
    const float separation = f.R * (0.25f + 1.45f * f.tear);
    f.lobeCentre[0] = f.numLobes == 2 ? polar (f.centre, f.tearAxis + kPi, separation * 0.45f * tearShown) : f.centre;
    f.lobeCentre[1] = polar (f.centre, f.tearAxis, separation * 0.55f * tearShown);
    f.lobeScale[0] = 1.0f - 0.2f * f.tear;
    f.lobeScale[1] = 0.76f - 0.12f * f.tear;
    f.lobeAlpha[0] = 1.0f;
    f.lobeAlpha[1] = tearShown;

    // Outline geometry for lobes and their refraction shells.
    ObjectField::Params p;
    p.density = f.density; p.form = f.form; p.surface = f.surface; p.bend = f.bend; p.magnet = f.magnet;
    p.melt = f.melt; p.crush = f.crush; p.fracture = f.fracture; p.tension = f.tension;
    p.rotation = rotation; p.time = time;
    p.stretchAxis = -0.35f + 0.4f * std::sin (time * 0.07f);
    p.bendAxis = time * 0.09f + 0.8f;
    p.facets = 5.0f + 3.0f * f.pitchU;
    p.symmetry = 3.0f + 3.0f * f.pitchU;
    p.breathe = breathe;
    for (int L = 0; L < f.numLobes; ++L)
    {
        p.phase = 11.0f * (float) L;
        field.buildOutline (p, noise, f.lobeCentre[L], f.R * f.lobeScale[L], lobes[(size_t) L]);
        for (int s = 0; s < f.numShells; ++s)
        {
            // Shells are offset, not concentric: refracted light bands drifting through the glass.
            ObjectField::Params sp = p;
            sp.phase = p.phase + 3.7f * (float) (s + 1);
            sp.rotation = rotation * (1.0f + 0.35f * (float) (s + 1)) + 0.4f * (float) s;
            sp.surface *= 0.4f;
            sp.fracture *= 0.5f;
            sp.bend *= 0.5f;
            const float scale = (0.80f - 0.16f * (float) s) * f.lobeScale[L];
            const float dir = rotation * (0.7f + 0.3f * (float) s) + 2.1f * (float) s + 0.6f;
            const auto sc = polar (f.lobeCentre[L], dir, f.R * f.lobeScale[L] * (0.09f + 0.05f * (float) s) * (1.0f + 0.8f * f.bend));
            field.buildOutline (sp, noise, sc, f.R * scale, shells[(size_t) L][(size_t) s]);
        }
    }
    mark (4);

    // ---- Layers
    drawBackdrop (g, f);
    drawStars (g, f);               mark (1);
    drawFragments (g, f, false);    mark (2);
    if (f.numLobes == 2) { drawLobe (g, f, 1); mark (4); }
    drawLobe (g, f, 0);             mark (4);
    if (f.numLobes == 2) { drawStrands (g, f); mark (3); }
    drawSparks (g, f);              mark (12);
    drawFragments (g, f, true);     mark (13);
    drawNodes (g, f);               mark (14);
    drawCaptions (g, f.bounds);     mark (15);

    const double ms = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1000.0;
    paintMsAverage = paintMsAverage <= 0.0f ? (float) ms : paintMsAverage + ((float) ms - paintMsAverage) * 0.08f;
    for (int i = 0; i < kLayers; ++i)
        layerMsAverage[(size_t) i] += (frameLayerMs[(size_t) i] - layerMsAverage[(size_t) i]) * 0.08;
}

//==============================================================================
void AntiMatterVisualizer::drawBackdrop (juce::Graphics& g, const Frame& f)
{
    // The halo, plasma aura and outer glow are the most expensive soft fills.
    // They are blur-like by nature, so they are rendered at half resolution
    // into an image every few frames and upscaled with a plain blit.
    const float deviceScale = juce::jmax (0.5f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const float scale = deviceScale * kBackdropRes;
    const int fullW = juce::jmax (2, juce::roundToInt (f.bounds.getWidth() * deviceScale));
    const int fullH = juce::jmax (2, juce::roundToInt (f.bounds.getHeight() * deviceScale));
    const int w = (fullW + 1) / 2, h = (fullH + 1) / 2;
    if (backdrop.isNull() || backdrop.getWidth() != fullW || backdrop.getHeight() != fullH || std::abs (backdropScale - scale) > 0.01f)
    {
        backdropSmall = juce::Image (juce::Image::ARGB, w, h, true);
        backdrop = juce::Image (juce::Image::ARGB, fullW, fullH, true);
        backdropScale = scale;
        backdropAge = 1000;
    }
    if (++backdropAge >= f.caps.backdropInterval)
    {
        backdropAge = 0;
        backdropSmall.clear (backdropSmall.getBounds());
        juce::Graphics ig (backdropSmall);
        ig.addTransform (juce::AffineTransform::scale (scale));
        renderBackdrop (ig, f);
        upscaleBackdrop();
    }
    mark (0);
    // Translation-only image draw: JUCE's fast blit path (the transformed path costs ~4x).
    g.drawImageTransformed (backdrop, juce::AffineTransform::scale (1.0f / deviceScale), false);
    mark (16);
}

void AntiMatterVisualizer::upscaleBackdrop() noexcept
{
    // 2x nearest-neighbour upscale of premultiplied ARGB; the content is blur-like so blocks are invisible.
    juce::Image::BitmapData src (backdropSmall, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (backdrop, juce::Image::BitmapData::writeOnly);
    const int srcW = src.width, dstW = dst.width;
    const size_t rowBytes = (size_t) dstW * 4;
    for (int y = 0; y < dst.height; y += 2)
    {
        const juce::uint8* s = src.getLinePointer (juce::jmin (src.height - 1, y >> 1));
        juce::uint8* d = dst.getLinePointer (y);
        int x = 0;
        for (int sx = 0; sx < srcW && x + 1 < dstW; ++sx, x += 2)
        {
            std::memcpy (d + x * 4, s + sx * 4, 4);
            std::memcpy (d + x * 4 + 4, s + sx * 4, 4);
        }
        for (; x < dstW; ++x) std::memcpy (d + x * 4, s + (srcW - 1) * 4, 4);
        if (y + 1 < dst.height) std::memcpy (dst.getLinePointer (y + 1), d, rowBytes);
    }
}

void AntiMatterVisualizer::renderBackdrop (juce::Graphics& g, const Frame& f)
{
    const auto c = f.centre;

    // Environmental halo tinted by the Space type; activity widens and brightens it.
    {
        const float strength = f.space.strength * (0.55f + 0.45f * f.life);
        const float a = (0.10f + 0.16f * f.spaceActivity + 0.04f * f.pulse) * strength;
        const float radius = f.side * (0.62f + 0.22f * f.spaceActivity);
        juce::ColourGradient halo (alpha (f.space.tint, a), c.x, c.y, alpha (f.space.tint, 0.0f), c.x + radius, c.y, true);
        halo.addColour (0.35, alpha (f.space.tint, a * 0.55f));
        g.setGradientFill (halo);
        g.fillRect (f.bounds);
    }

    // Plasma aura: soft drifting clouds around the body — the "dark matter" volume the glass sits in.
    {
        const juce::Colour tints[3] = { Theme::violet, Theme::blue, Theme::magenta };
        const int clouds = f.caps.shells >= 2 ? 3 : 2;
        for (int k = 0; k < clouds; ++k)
        {
            const float phi = (float) k * kTwoPi / 3.0f + f.time * 0.06f + f.rotation * 0.3f;
            const float n1 = noise.noise (f.time * 0.12f + (float) k * 5.0f, 2.2f);
            const float n2 = noise.noise (3.1f, f.time * 0.1f + (float) k * 7.0f);
            const float dist = f.R * (0.28f + 0.16f * n1 + 0.25f * f.tear + 0.1f * f.bend);
            const auto cc = polar (c, phi, dist);
            const float radius = f.R * (1.05f + 0.18f * n2 + 0.35f * f.melt + 0.2f * f.density + 0.15f * f.pulse);
            const float a = (0.09f + 0.06f * f.pulse + 0.04f * f.life + 0.08f * f.melt) * (1.0f - 0.22f * (float) k);
            juce::ColourGradient cloud (alpha (tints[k], a), cc.x, cc.y, alpha (tints[k], 0.0f), cc.x + radius, cc.y, true);
            cloud.addColour (0.45, alpha (tints[k], a * 0.5f));
            g.setGradientFill (cloud);
            g.fillEllipse (cc.x - radius, cc.y - radius, radius * 2.0f, radius * 2.0f);
        }
        // Dark matter shadow: the volume the object bends light around.
        const float sr = f.R * (1.25f + 0.2f * f.mass);
        juce::ColourGradient shadow (alpha (juce::Colour (0xff04040a), 0.55f + 0.25f * f.mass), c.x, c.y, alpha (juce::Colour (0xff04040a), 0.0f), c.x + sr, c.y, true);
        shadow.addColour (0.5, alpha (juce::Colour (0xff04040a), 0.35f));
        g.setGradientFill (shadow);
        g.fillEllipse (c.x - sr, c.y - sr, sr * 2.0f, sr * 2.0f);

        // Outer glow: the glass edge scattering light into the aura.
        for (int L = 0; L < f.numLobes; ++L)
        {
            const auto lc = f.lobeCentre[L];
            const float gr = f.R * f.lobeScale[L] * (1.16f + 0.3f * f.melt);
            const float ga = (0.13f + 0.08f * f.pulse) * f.lobeAlpha[L];
            juce::ColourGradient glow (alpha (Theme::violet, ga), lc.x, lc.y, alpha (Theme::violet, 0.0f), lc.x + gr, lc.y, true);
            glow.addColour (0.80, alpha (Theme::violet, ga * 0.9f));
            g.setGradientFill (glow);
            g.fillEllipse (lc.x - gr, lc.y - gr, gr * 2.0f, gr * 2.0f);
        }
    }
}

void AntiMatterVisualizer::drawStars (juce::Graphics& g, const Frame& f)
{
    const int count = f.caps.sparks > 0 ? ParticleSystem::kMaxStars : ParticleSystem::kMaxStars / 2;
    const float drift = f.time * 0.0015f;
    for (int i = 0; i < count; ++i)
    {
        const auto& s = particles.stars[(size_t) i];
        const float tw = 0.5f + 0.5f * std::sin (f.time * (0.6f + 0.4f * s.size) + s.twinkle);
        float x = f.bounds.getX() + std::fmod (s.x + drift * s.size, 1.0f) * f.bounds.getWidth();
        float y = f.bounds.getY() + s.y * f.bounds.getHeight();
        const float size = s.size * f.px;
        g.setColour (alpha (i % 7 == 0 ? Theme::cyan : Theme::textPrimary, 0.05f + 0.22f * tw * s.size / 1.9f));
        g.fillRect (x, y, size, size);
    }
}

//==============================================================================
void AntiMatterVisualizer::drawFragments (juce::Graphics& g, const Frame& f, bool nearLayer)
{
    const auto c = f.centre;
    const float shardBoost = 1.0f + 0.7f * f.fracture;
    for (int i = 0; i < f.numFragments; ++i)
    {
        const auto& fr = particles.fragments[(size_t) i];
        const bool isNear = fr.depth >= 0.0f;
        if (isNear != nearLayer) continue;

        const juce::Point<float> p (c.x + fr.x * f.R, c.y + fr.y * f.R);
        const float depthMix = 0.5f + 0.5f * fr.depth;                       // 0 far .. 1 near
        const float s = fr.size * f.R * fr.scale * shardBoost * (1.0f + 0.25f * f.pulse) * 1.25f;
        const float a = (0.3f + 0.7f * depthMix) * (0.65f + 0.35f * f.life) * (1.0f - 0.5f * f.melt);
        const float spin = f.time * fr.spin * (0.4f + 0.6f * f.decay + f.fracture) + fr.spinPhase;
        const float hue = fr.hue + f.hue * 0.5f;
        if (s < 0.6f) continue;

        // Decay → motion streak (afterglow) from the oldest recorded position.
        if (f.caps.streaks && f.decay > 0.05f)
        {
            const auto old = ParticleSystem::oldest (fr);
            const juce::Point<float> q (c.x + old.x * f.R, c.y + old.y * f.R);
            const float len = p.getDistanceFrom (q);
            if (len > 0.6f)
            {
                g.setColour (alpha (iridescence.at (hue), 0.35f * f.decay * a * juce::jmin (1.0f, len / (4.0f * f.px))));
                g.drawLine (juce::Line<float> (q, p), juce::jmax (0.6f, s * 0.5f));
            }
        }

        glyph.clear();
        for (int k = 0; k < fr.sides; ++k)
        {
            const float ka = (float) k / (float) fr.sides * kTwoPi + spin;
            const float kr = s * fr.vertexRadius[(size_t) k];
            const auto v = polar (p, ka, kr);
            if (k == 0) glyph.startNewSubPath (v); else glyph.lineTo (v);
        }
        glyph.closeSubPath();

        // Glassy fill: a gradient across the shard, then a bright thin edge.
        const auto colA = iridescence.lit (hue, 0.2f), colB = iridescence.at (hue + 0.33f);
        juce::ColourGradient fill (alpha (colA, 0.50f * a), p.x - s, p.y - s, alpha (colB, 0.12f * a), p.x + s, p.y + s, false);
        g.setGradientFill (fill);
        g.fillPath (glyph);
        if (nearLayer && f.caps.fragmentGlow && s > 3.0f)
        {
            g.setColour (alpha (colA, 0.12f * a));
            g.strokePath (glyph, juce::PathStrokeType (s * 0.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        g.setColour (alpha (iridescence.lit (hue, 0.45f + 0.3f * depthMix), (0.6f + 0.4f * depthMix) * a));
        g.strokePath (glyph, juce::PathStrokeType (juce::jmax (0.7f, 0.9f * f.px)));
        if (fr.sides >= 4 && s > 3.0f)
        {
            // an internal facet edge so the shard reads as a crystal, not a flat polygon
            const auto v0 = polar (p, spin, s * fr.vertexRadius[0]);
            const auto v2 = polar (p, spin + (float) (fr.sides / 2) / (float) fr.sides * kTwoPi, s * fr.vertexRadius[(size_t) (fr.sides / 2)]);
            g.setColour (alpha (Theme::ivory, (0.25f + 0.25f * depthMix) * a));
            g.drawLine (juce::Line<float> (v0, v2), juce::jmax (0.6f, 0.7f * f.px));
        }
        if (nearLayer && s > 4.0f)
        {
            // one lit facet corner
            const auto v = polar (p, spin, s * fr.vertexRadius[0]);
            const float d = juce::jmax (1.0f, 1.4f * f.px);
            g.setColour (alpha (Theme::ivory, 0.8f * a));
            g.fillEllipse (v.x - d * 0.5f, v.y - d * 0.5f, d, d);
        }
    }
}

void AntiMatterVisualizer::drawSparks (juce::Graphics& g, const Frame& f)
{
    const auto c = f.centre;
    for (int i = 0; i < f.numSparks; ++i)
    {
        const auto& s = particles.sparks[(size_t) i];
        const float size = (0.9f + 1.4f * s.size) * f.px * (1.0f + 0.4f * f.pulse);
        const float a = s.brightness * (0.35f + 0.65f * f.life) * 0.9f;
        const juce::Point<float> p (c.x + s.x * f.R, c.y + s.y * f.R);
        g.setColour (alpha (iridescence.lit (s.hue + f.hue, 0.3f), a));
        g.fillEllipse (p.x - size * 0.5f, p.y - size * 0.5f, size, size);
    }
}

//==============================================================================
void AntiMatterVisualizer::drawLobe (juce::Graphics& g, const Frame& f, int L)
{
    const auto& o = lobes[(size_t) L];
    const float A = f.lobeAlpha[L];
    const float R = o.baseRadius;
    const auto c = o.centre;
    ObjectField::toPath (o, bodyPath);

    // 1. Volume: dark liquid glass. Lit from the top-left (cold blue) with a
    //    warm magenta-violet bounce from the bottom-right, denser with Mass.
    {
        const float dens = 0.78f + 0.18f * f.mass - 0.2f * f.melt;
        const juce::Point<float> lit (c.x - R * 0.85f, c.y - R * 0.85f), shade (c.x + R * 0.85f, c.y + R * 0.85f);
        juce::ColourGradient vol (alpha (juce::Colour (0xff1f2f78), (dens + 0.02f) * A), lit.x, lit.y,
                                  alpha (juce::Colour (0xff241040), (dens + 0.06f) * A), shade.x, shade.y, false);
        vol.addColour (0.40, alpha (juce::Colour (0xff08081c), (dens + 0.14f) * A));
        vol.addColour (0.64, alpha (juce::Colour (0xff0a0722), (dens + 0.12f) * A));
        g.setGradientFill (vol);
        g.fillPath (bodyPath);

        // Dark matter clots: soft near-black turbulence drifting inside the glass (Mass makes them heavier).
        for (int k = 0; k < 3; ++k)
        {
            const float fk = (float) k;
            const float ang = f.rotation * (0.8f + 0.25f * fk) + fk * 2.1f + 0.3f * noise.noise (fk * 3.0f, f.time * 0.15f);
            const float dist = R * (0.42f + 0.16f * noise.noise (f.time * 0.1f, fk * 5.0f + 2.0f));
            const auto cc = polar (c, ang, dist);
            const float rr = R * (0.24f + 0.08f * f.mass + 0.06f * f.density);
            juce::ColourGradient clot (alpha (juce::Colour (0xff030309), (0.30f + 0.25f * f.mass) * A), cc.x, cc.y, alpha (juce::Colour (0xff030309), 0.0f), cc.x + rr, cc.y, true);
            g.setGradientFill (clot);
            g.fillEllipse (cc.x - rr, cc.y - rr, rr * 2.0f, rr * 2.0f);
        }

        // Plasma wisps: iridescent colour drifting through the glass (Density adds wisps, audio brightens them).
        const int wisps = 2 + (int) (f.density * 2.0f);
        for (int k = 0; k < wisps; ++k)
        {
            const float fk = (float) k;
            const float ang = -f.rotation * (0.6f + 0.2f * fk) + fk * 1.9f + 0.5f * noise.noise (fk * 4.0f + 1.0f, f.time * 0.12f);
            const float dist = R * (0.30f + 0.25f * (0.5f + 0.5f * noise.noise (f.time * 0.09f + fk, fk * 6.0f)));
            const auto cc = polar (c, ang, dist);
            const float rr = R * (0.22f + 0.05f * (float) (k % 2));
            const auto col = iridescence.at (f.hue + 0.27f * fk + 0.1f);
            juce::ColourGradient wisp (alpha (col, (0.17f + 0.1f * f.pulse) * (1.0f - 0.4f * f.melt) * A), cc.x, cc.y, alpha (col, 0.0f), cc.x + rr, cc.y, true);
            g.setGradientFill (wisp);
            g.fillEllipse (cc.x - rr, cc.y - rr, rr * 2.0f, rr * 2.0f);
        }

        // Fresnel body glow: transparent centre, tinted toward the edge (glass thickness).
        juce::ColourGradient fres (alpha (Theme::blue, 0.0f), c.x, c.y, alpha (Theme::violet, (0.45f + 0.2f * f.pulse) * A), c.x + R * 1.02f, c.y, true);
        fres.addColour (0.58, alpha (Theme::blue, 0.0f));
        fres.addColour (0.86, alpha (Theme::blue, (0.20f + 0.1f * f.pulse) * A));
        g.setGradientFill (fres);
        g.fillPath (bodyPath);

        if (f.melt > 0.02f)
        {
            // Melt: a diffuse copy bleeding outward.
            ObjectField::toPath (o, shellPath, 1.0f + 0.14f * f.melt);
            g.setColour (alpha (Theme::violet, 0.16f * f.melt * A));
            g.fillPath (shellPath);
        }
    }
    mark (5);
    drawShells (g, f, L);       mark (6);
    drawFilaments (g, f, L);    mark (7);
    drawCore (g, f, L);         mark (8);

    // Specular highlights: the key light reflected in liquid glass — a soft sheen and a sharp point.
    {
        const float hx = R * 0.40f * (1.0f - 0.3f * f.melt), hy = R * 0.17f;
        const auto hc = juce::Point<float> (c.x - R * 0.40f, c.y - R * 0.46f);
        glyph.clear();
        glyph.addEllipse (hc.x - hx, hc.y - hy, hx * 2.0f, hy * 2.0f);
        glyph.applyTransform (juce::AffineTransform::rotation (-0.66f, hc.x, hc.y));
        const auto sheen = Theme::ivory.interpolatedWith (Theme::cyan, 0.35f);
        juce::ColourGradient hl (alpha (sheen, (0.36f + 0.18f * f.pulse) * (1.0f - 0.5f * f.melt) * A), hc.x, hc.y, alpha (sheen, 0.0f), hc.x + hx, hc.y, true);
        hl.addColour (0.4, alpha (sheen, 0.12f * A));
        g.setGradientFill (hl);
        g.fillPath (glyph);

        const auto sp = juce::Point<float> (c.x - R * 0.52f, c.y - R * 0.56f);
        const float sr = R * 0.045f * (1.0f + 0.4f * f.pulse);
        juce::ColourGradient spot (alpha (Theme::ivory, (0.85f - 0.5f * f.melt) * A), sp.x, sp.y, alpha (Theme::ivory, 0.0f), sp.x + sr * 2.2f, sp.y, true);
        spot.addColour (0.3, alpha (Theme::ivory, 0.5f * A));
        g.setGradientFill (spot);
        g.fillEllipse (sp.x - sr * 2.2f, sp.y - sr * 2.2f, sr * 4.4f, sr * 4.4f);
    }
    mark (9);
    drawRim (g, f, L);          mark (10);
    if (f.fracture > 0.03f) drawCracks (g, f, L);
    mark (11);
}

void AntiMatterVisualizer::drawShells (juce::Graphics& g, const Frame& f, int L)
{
    const auto& o = lobes[(size_t) L];
    const float A = f.lobeAlpha[L];
    const auto c = o.centre;
    const float R = o.baseRadius;

    // Refraction crescents: offset shells filled with a directional gradient (bright far side → clear near side).
    for (int s = 0; s < f.numShells; ++s)
    {
        const auto& sh = shells[(size_t) L][(size_t) s];
        ObjectField::toPath (sh, shellPath);
        const float hue = f.hue + 0.31f * (float) s + 0.1f * (float) L + 0.05f;
        const auto ca = iridescence.lit (hue, 0.15f), cb = iridescence.at (hue + 0.5f);
        const float dir = std::atan2 (sh.centre.y - c.y, sh.centre.x - c.x);
        const auto p1 = polar (sh.centre, dir, sh.baseRadius), p2 = polar (sh.centre, dir + kPi, sh.baseRadius * 0.6f);
        const float fillA = (0.20f + 0.08f * f.pulse + 0.08f * f.melt) * A * (1.0f - 0.15f * (float) s);
        juce::ColourGradient grad (alpha (ca, fillA), p1.x, p1.y, alpha (cb, 0.0f), p2.x, p2.y, false);
        grad.addColour (0.5, alpha (ca.interpolatedWith (cb, 0.5f), fillA * 0.35f));
        g.setGradientFill (grad);
        g.fillPath (shellPath);
        // hairline refraction edge, strongest on the outer shell
        if (s == 0)
        {
            const float edgeA = 0.22f * (1.0f + 0.5f * f.pulse) * (1.0f - 0.7f * f.melt) * A;
            g.setColour (alpha (iridescence.lit (hue + 0.05f, 0.3f), edgeA));
            g.strokePath (shellPath, juce::PathStrokeType (juce::jmax (0.6f, 0.75f * f.px)));
        }
    }

    // Swirl bands: light curling around the well inside the glass (the marble's "cat's eye").
    {
        const int bands = f.numShells >= 2 ? 2 : 1;
        for (int b = 0; b < bands; ++b)
        {
            const float fb = (float) b;
            const float rr = R * (0.50f + 0.2f * fb) * (1.0f + 0.15f * f.bend);
            const float a0 = f.rotation * (1.3f + 0.4f * fb) + fb * 2.6f;
            const float span = 2.2f + 0.6f * f.density - 0.5f * f.form;
            ringPath.clear();
            ringPath.addCentredArc (c.x, c.y, rr, rr * (0.8f + 0.2f * f.magnet), 0.35f * fb, a0, a0 + span, true);
            const auto q0 = polar (c, a0 - kPi * 0.5f, rr), q1 = polar (c, a0 + span - kPi * 0.5f, rr);
            const auto ca = iridescence.lit (f.hue + 0.6f + 0.3f * fb, 0.25f), cb = iridescence.at (f.hue + 0.15f + 0.3f * fb);
            juce::ColourGradient grad (alpha (ca, (0.13f + 0.06f * f.pulse) * A), q0.x, q0.y, alpha (cb, 0.01f * A), q1.x, q1.y, false);
            g.setGradientFill (grad);
            g.strokePath (ringPath, juce::PathStrokeType (R * (0.12f - 0.02f * fb) * (1.0f + 0.5f * f.melt), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (alpha (ca, (0.28f + 0.15f * f.pulse) * A * (1.0f - 0.6f * f.melt)));
            g.strokePath (ringPath, juce::PathStrokeType (juce::jmax (0.6f, 0.8f * f.px), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

void AntiMatterVisualizer::drawFilaments (juce::Graphics& g, const Frame& f, int L)
{
    const auto& o = lobes[(size_t) L];
    const float A = f.lobeAlpha[L];
    const auto c = o.centre;
    const float coreR = f.coreR * f.lobeScale[L] * (L == 0 ? 1.0f : 0.7f);
    const int count = L == 0 ? f.numFilaments : juce::jmax (2, f.numFilaments / 2);
    const float align = 1.0f - 0.85f * f.magnet;
    const float vitality = 0.3f + 0.7f * f.life;

    for (int i = 0; i < count; ++i)
    {
        const float fi = (float) i;
        const float h = hash01 (i + 7 * L);
        const float a0 = f.rotation * 0.6f + fi * kTwoPi / (float) count + 0.5f * align * noise.noise (fi * 1.3f + 0.5f, f.time * 0.22f);
        const float curl = (0.5f + 1.3f * f.bend) * std::sin (f.time * 0.3f + fi * 1.7f) * align + 0.6f * f.bend;
        const float a1 = a0 + curl;
        const float idx = std::fmod ((a1 / kTwoPi) * (float) ObjectField::kSamples + (float) ObjectField::kSamples * 4.0f, (float) ObjectField::kSamples);
        const float reach = (0.55f + 0.42f * h) * (1.0f - 0.1f * f.melt) * (0.9f + 0.1f * f.energy);
        const auto rimPoint = ObjectField::pointAt (o, idx, 1.0f);
        const float rimR = rimPoint.getDistanceFrom (c);
        const float endR = juce::jmin (rimR * 0.93f, o.baseRadius * reach);
        const auto end = polar (c, a1, endR);
        const auto start = polar (c, a0, coreR * 1.04f);
        const float wob = f.R * 0.24f * align * (0.5f + 0.7f * f.surface);
        const auto m1 = polar (c, a0 + curl * 0.3f, coreR + (endR - coreR) * 0.33f);
        const auto m2 = polar (c, a0 + curl * 0.72f, coreR + (endR - coreR) * 0.7f);
        const float n1 = noise.noise (fi * 2.1f, f.time * 0.6f + 1.0f), n2 = noise.noise (f.time * 0.5f, fi * 2.7f + 4.0f);
        curve.clear();
        curve.startNewSubPath (start);
        curve.cubicTo (m1.x + wob * n1, m1.y + wob * n2, m2.x - wob * n2, m2.y + wob * n1, end.x, end.y);

        const float flicker = 0.55f + 0.45f * noise.noise (fi * 3.3f, f.time * (1.5f + 4.0f * f.life));
        const float b = (0.35f + 0.65f * f.energy) * flicker * vitality * A * (1.0f - 0.45f * f.melt);
        const auto col = iridescence.lit (fi / (float) count + f.hue * 0.7f + 0.55f, 0.35f);
        if (f.caps.filamentHalo)
        {
            juce::ColourGradient halo (alpha (col, 0.36f * b + 0.1f * f.pulse), start.x, start.y, alpha (col, 0.0f), end.x, end.y, false);
            g.setGradientFill (halo);
            g.strokePath (curve, juce::PathStrokeType (juce::jmax (3.0f, 6.0f * f.px), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        juce::ColourGradient line (alpha (col.interpolatedWith (Theme::ivory, 0.5f), 0.6f + 0.4f * b), start.x, start.y, alpha (col, 0.08f + 0.25f * b), end.x, end.y, false);
        g.setGradientFill (line);
        g.strokePath (curve, juce::PathStrokeType (juce::jmax (0.9f, 1.5f * f.px), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // the spark where the thread leaves the well
        const float d = juce::jmax (1.2f, (1.8f + 1.2f * f.pulse) * f.px);
        g.setColour (alpha (Theme::ivory, (0.5f + 0.5f * b) * A));
        g.fillEllipse (start.x - d * 0.5f, start.y - d * 0.5f, d, d);
    }
}

void AntiMatterVisualizer::drawCore (juce::Graphics& g, const Frame& f, int L)
{
    const auto& o = lobes[(size_t) L];
    const float A = f.lobeAlpha[L] * (L == 0 ? 1.0f : smoothstep (0.35f, 0.9f, f.tear));
    if (A <= 0.01f) return;
    const float cr = f.coreR * f.lobeScale[L] * (L == 0 ? 1.0f : 0.7f);
    const auto c = juce::Point<float> (o.centre.x + f.bend * f.R * 0.08f * std::cos (f.time * 0.09f + 0.8f), o.centre.y + f.bend * f.R * 0.06f);

    // The void and its gravity well: a hole in the light, swallowing the glass around it.
    {
        const float vr = cr * 2.15f;
        const juce::Colour black (0xff020208), navy (0xff05041a);
        juce::ColourGradient dark (alpha (black, 0.97f * A), c.x, c.y, alpha (navy, 0.0f), c.x + vr, c.y, true);
        dark.addColour (0.42, alpha (black, 0.96f * A));
        dark.addColour (0.53, alpha (navy, 0.72f * A));
        dark.addColour (0.75, alpha (navy, 0.30f * A));
        g.setGradientFill (dark);
        g.fillEllipse (c.x - vr, c.y - vr, vr * 2.0f, vr * 2.0f);
    }

    // Gravitational lens ring: light bent around the void, iridescent, brightest where it is lensed.
    {
        const int segs = juce::jmax (32, f.caps.rimSegments / 2);
        const float w = juce::jmax (1.0f, 1.5f * f.px) * (1.0f + 0.6f * f.pulse) * 0.5f;
        const float shimmerT = f.time * 1.8f;
        ringPath.clear();
        ringPath.addEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
        // soft glow under the ring
        g.setColour (alpha (Theme::cyan, (0.16f + 0.22f * f.pulse + 0.06f * f.energy) * A));
        g.strokePath (ringPath, juce::PathStrokeType (juce::jmax (3.5f, 10.0f * f.px)));
        g.setColour (alpha (Theme::ivory, (0.18f + 0.25f * f.pulse) * A));
        g.strokePath (ringPath, juce::PathStrokeType (juce::jmax (1.5f, 3.6f * f.px)));
        for (int i = 0; i < segs; ++i)
        {
            const float a0 = (float) i / (float) segs * kTwoPi, a1 = (float) (i + 1) / (float) segs * kTwoPi + 0.004f;
            const float am = (a0 + a1) * 0.5f;
            const float lens = 0.35f + 0.65f * std::pow (0.5f + 0.5f * std::cos (2.0f * am + 0.9f + f.rotation * 2.0f), 2.0f);   // two bright lensing lobes
            const float shimmer = 0.85f + 0.15f * std::sin (am * 6.0f + shimmerT);
            const float bright = sat (lens * shimmer * (0.75f + 0.35f * f.pulse + 0.2f * f.energy));
            const auto col = iridescence.lit (am / kTwoPi * 2.0f - f.hue * 2.0f + 0.15f, 0.25f + 0.65f * bright);
            const float wi = w * (0.7f + 0.6f * lens);
            quad.clear();
            quad.startNewSubPath (c.x + std::cos (a0) * (cr - wi), c.y + std::sin (a0) * (cr - wi));
            quad.lineTo (c.x + std::cos (a1) * (cr - wi), c.y + std::sin (a1) * (cr - wi));
            quad.lineTo (c.x + std::cos (a1) * (cr + wi), c.y + std::sin (a1) * (cr + wi));
            quad.lineTo (c.x + std::cos (a0) * (cr + wi), c.y + std::sin (a0) * (cr + wi));
            quad.closeSubPath();
            g.setColour (alpha (col, (0.45f + 0.55f * bright) * A));
            g.fillPath (quad);
        }
    }

    // Einstein arcs: thin secondary rings, their number set by pitch (higher notes → more, tighter arcs).
    {
        const int arcs = 1 + (int) (f.pitchU * 3.0f);
        for (int k = 0; k < arcs; ++k)
        {
            const float ar = cr * (1.25f + 0.19f * (float) k) * (1.0f + 0.03f * std::sin (f.time * 1.3f + (float) k));
            const float span = 1.7f + 0.9f * f.magnet - 0.3f * (float) k;
            const float a0 = f.time * (0.25f + 0.1f * (float) k) * (k % 2 == 0 ? 1.0f : -1.0f) + (float) k * 2.1f;
            ringPath.clear();
            ringPath.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, a0 + span, true);
            const float b = (0.42f - 0.08f * (float) k + 0.25f * f.pulse) * A * (0.5f + 0.5f * f.life);
            const auto col = k % 2 == 0 ? Theme::ivory : Theme::cyan;
            g.setColour (alpha (col, b * 0.3f));
            g.strokePath (ringPath, juce::PathStrokeType (juce::jmax (2.0f, 3.6f * f.px)));
            g.setColour (alpha (col, b));
            g.strokePath (ringPath, juce::PathStrokeType (juce::jmax (0.7f, 1.0f * f.px)));
        }
    }

    // A faint singularity glimmer off-centre — the light that did not escape.
    {
        const float gr = cr * 0.36f;
        const auto gc = polar (c, f.rotation * 1.5f + 2.0f, cr * 0.3f);
        juce::ColourGradient glim (alpha (Theme::violet, (0.16f + 0.25f * f.pulse) * A), gc.x, gc.y, alpha (Theme::violet, 0.0f), gc.x + gr, gc.y, true);
        g.setGradientFill (glim);
        g.fillEllipse (gc.x - gr, gc.y - gr, gr * 2.0f, gr * 2.0f);
    }
}

void AntiMatterVisualizer::drawRim (juce::Graphics& g, const Frame& f, int L)
{
    const auto& o = lobes[(size_t) L];
    const float A = f.lobeAlpha[L];
    const int segs = f.caps.rimSegments;
    const float meltSoft = 1.0f - 0.6f * f.melt;
    const float halfW = juce::jmax (0.7f, 1.1f * f.px) * (1.0f + 1.6f * f.melt) * (1.0f + 0.25f * f.pulse) * 0.5f;
    const float wideGlow = f.R * (0.05f + 0.06f * f.melt);

    // Soft rim glow (gradient stroke), then the thin iridescent Fresnel rim as coloured quads.
    {
        juce::ColourGradient grad (alpha (Theme::cyan, (0.18f + 0.1f * f.pulse) * meltSoft * A), o.centre.x - f.R, o.centre.y - f.R,
                                   alpha (Theme::magenta, (0.14f + 0.08f * f.pulse) * meltSoft * A), o.centre.x + f.R, o.centre.y + f.R, false);
        grad.addColour (0.5, alpha (Theme::violet, (0.14f + 0.08f * f.pulse) * meltSoft * A));
        g.setGradientFill (grad);
        g.strokePath (bodyPath, juce::PathStrokeType (wideGlow, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Inner reflection: the rim light bouncing inside the glass wall, opposite the key light.
    {
        ObjectField::toPath (o, shellPath, 0.94f - 0.03f * f.melt);
        juce::ColourGradient inner (alpha (Theme::ivory, 0.0f), o.centre.x - f.R * 0.6f, o.centre.y - f.R * 0.6f,
                                    alpha (Theme::ivory.interpolatedWith (Theme::magenta, 0.3f), (0.30f + 0.15f * f.pulse) * meltSoft * A), o.centre.x + f.R * 0.8f, o.centre.y + f.R * 0.8f, false);
        inner.addColour (0.45, alpha (Theme::ivory, 0.03f * A));
        g.setGradientFill (inner);
        g.strokePath (shellPath, juce::PathStrokeType (juce::jmax (0.6f, 0.8f * f.px)));
    }

    const float shimmerT = f.time * 2.2f;
    const float hueCycles = 1.5f + 0.7f * f.density;
    for (int i = 0; i < segs; ++i)
    {
        const int i0 = (i * ObjectField::kSamples) / segs;
        const int i1 = ((i + 1) * ObjectField::kSamples / segs) % ObjectField::kSamples;
        const auto& p0 = o.points[(size_t) i0];
        const auto& p1 = o.points[(size_t) i1];
        const auto& n0 = o.normals[(size_t) i0];
        const auto& n1 = o.normals[(size_t) i1];
        const float nx = (n0.x + n1.x) * 0.5f, ny = (n0.y + n1.y) * 0.5f;
        const float fres = Iridescence::fresnel (nx, ny);
        const float u = (float) i / (float) segs;
        const float shimmer = 0.82f + 0.18f * std::sin (u * kTwoPi * 5.0f + shimmerT) * (0.5f + 0.5f * f.surface);
        const float bright = sat ((0.22f + 0.78f * fres) * shimmer * (0.9f + 0.35f * f.pulse));
        const auto col = iridescence.lit (u * hueCycles + f.hue + 0.2f * noise.noise (u * 12.0f, f.time * 0.4f) * f.surface, sat ((fres - 0.55f) * 0.9f));
        // extend slightly along the tangent so anti-aliased seams never show
        const float tx = p1.x - p0.x, ty = p1.y - p0.y;
        const float tl = std::sqrt (tx * tx + ty * ty);
        const float ex = tl > 1.0e-4f ? tx / tl * 0.35f : 0.0f, ey = tl > 1.0e-4f ? ty / tl * 0.35f : 0.0f;
        const float wIn = halfW * (0.8f + 0.5f * fres), wOut = halfW * (0.8f + 0.5f * fres);
        quad.clear();
        quad.startNewSubPath (p0.x - n0.x * wIn - ex, p0.y - n0.y * wIn - ey);
        quad.lineTo (p1.x - n1.x * wIn + ex, p1.y - n1.y * wIn + ey);
        quad.lineTo (p1.x + n1.x * wOut + ex, p1.y + n1.y * wOut + ey);
        quad.lineTo (p0.x + n0.x * wOut - ex, p0.y + n0.y * wOut - ey);
        quad.closeSubPath();
        g.setColour (alpha (col, (0.3f + 0.7f * bright) * meltSoft * A));
        g.fillPath (quad);
    }

    // Surface → sparkles: micro-detail glints travelling along the rim.
    if (f.surface > 0.05f && f.caps.sparks > 0)
    {
        const int glints = 3 + (int) (f.surface * 12.0f);
        for (int k = 0; k < glints; ++k)
        {
            const float fk = (float) k;
            const float idx = std::fmod (fk * 37.0f + f.time * (6.0f + 9.0f * fk / (float) glints) * (0.3f + 0.7f * f.life) + 400.0f, (float) ObjectField::kSamples);
            const float tw = 0.5f + 0.5f * std::sin (f.time * 9.0f + fk * 2.3f);
            const auto p = ObjectField::pointAt (o, idx);
            const float d = (1.2f + 1.8f * tw) * f.px;
            g.setColour (alpha (Theme::ivory, (0.25f + 0.65f * tw) * f.surface * A));
            g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
        }
    }
}

void AntiMatterVisualizer::drawCracks (juce::Graphics& g, const Frame& f, int L)
{
    const auto& o = lobes[(size_t) L];
    const float A = f.lobeAlpha[L] * f.fracture;
    const int cracks = juce::jlimit (1, 7, (int) std::lround (f.fracture * 7.0f));
    for (int k = 0; k < cracks; ++k)
    {
        const float fk = (float) k;
        const float drift = noise.noise (fk * 4.3f + 2.0f, f.time * 0.35f);
        const float idx = std::fmod (fk / (float) cracks * (float) ObjectField::kSamples + drift * 12.0f + f.rotation * 20.0f + 512.0f, (float) ObjectField::kSamples);
        const auto start = ObjectField::pointAt (o, idx, 1.0f);
        curve.clear();
        curve.startNewSubPath (start);
        juce::Point<float> p = start;
        const int steps = 4;
        for (int s = 1; s <= steps; ++s)
        {
            const float t = (float) s / (float) steps;
            const float reach = (0.45f + 0.35f * noise.noise (fk * 9.1f, f.time * 0.8f + t)) * t;
            const juce::Point<float> tgt (start.x + (o.centre.x - start.x) * reach, start.y + (o.centre.y - start.y) * reach);
            const float jag = f.R * 0.09f * noise.noise (fk * 5.5f + t * 7.0f, f.time * 1.6f);
            const float nx = -(o.centre.y - start.y), ny = (o.centre.x - start.x);
            const float nl = std::sqrt (nx * nx + ny * ny);
            p = { tgt.x + (nl > 0.0f ? nx / nl : 0.0f) * jag, tgt.y + (nl > 0.0f ? ny / nl : 0.0f) * jag };
            curve.lineTo (p);
        }
        const float flick = 0.5f + 0.5f * noise.noise (fk * 13.0f, f.time * 7.0f);
        const auto col = k % 2 == 0 ? Theme::magenta : Theme::ivory;
        g.setColour (alpha (col, 0.14f * flick * A));
        g.strokePath (curve, juce::PathStrokeType (juce::jmax (2.0f, 4.0f * f.px)));
        g.setColour (alpha (col, (0.45f + 0.5f * flick) * A));
        g.strokePath (curve, juce::PathStrokeType (juce::jmax (0.6f, 0.9f * f.px)));
    }
}

void AntiMatterVisualizer::drawStrands (juce::Graphics& g, const Frame& f)
{
    // Taffy strands between the two torn lobes: stretched matter that has not let go yet.
    const float show = smoothstep (0.06f, 0.4f, f.tear) * (1.0f - 0.7f * smoothstep (0.85f, 1.0f, f.tear));
    if (show <= 0.01f) return;
    const auto& a = lobes[0];
    const auto& b = lobes[1];
    const float idxA = (f.tearAxis / kTwoPi) * (float) ObjectField::kSamples;
    const float idxB = ((f.tearAxis + kPi) / kTwoPi) * (float) ObjectField::kSamples;
    const int strands = 3 + (int) (f.density * 4.0f);
    for (int k = 0; k < strands; ++k)
    {
        const float spread = ((float) k / (float) juce::jmax (1, strands - 1) - 0.5f) * 22.0f;
        const auto p0 = ObjectField::pointAt (a, idxA + spread, 0.97f);
        const auto p1 = ObjectField::pointAt (b, idxB - spread, 0.97f);
        const juce::Point<float> mid ((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
        const float sag = f.R * (0.08f + 0.2f * f.melt) * noise.noise ((float) k * 3.1f, f.time * 0.7f);
        const float nx = -(p1.y - p0.y), ny = (p1.x - p0.x);
        const float nl = juce::jmax (1.0e-4f, std::sqrt (nx * nx + ny * ny));
        const juce::Point<float> ctrl (mid.x + nx / nl * sag, mid.y + ny / nl * sag + f.R * 0.05f * f.melt);
        curve.clear();
        curve.startNewSubPath (p0);
        curve.quadraticTo (ctrl, p1);
        const auto col = iridescence.lit ((float) k * 0.13f + f.hue, 0.35f);
        g.setColour (alpha (col, 0.12f * show));
        g.strokePath (curve, juce::PathStrokeType (juce::jmax (2.0f, 4.0f * f.px), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (alpha (col, (0.4f + 0.3f * f.energy) * show));
        g.strokePath (curve, juce::PathStrokeType (juce::jmax (0.6f, 0.9f * f.px), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
void AntiMatterVisualizer::drawNodes (juce::Graphics& g, const Frame& f)
{
    const auto c = f.centre;
    const float ringR = f.R * 1.28f;

    // Octave dial: faint ring with ticks every octave (30 Hz .. 15 kHz), the scale the node markers use.
    {
        g.setColour (alpha (juce::Colours::white, 0.05f));
        g.drawEllipse (c.x - ringR, c.y - ringR, ringR * 2.0f, ringR * 2.0f, 1.0f);
        for (int k = 0; k < 9; ++k)
        {
            const float a = -kPi * 0.5f + (float) k / 9.0f * kTwoPi;
            const float len = (k % 3 == 0 ? 7.0f : 4.0f) * f.px;
            g.setColour (alpha (juce::Colours::white, k % 3 == 0 ? 0.16f : 0.09f));
            g.drawLine (juce::Line<float> (polar (c, a, ringR - len * 0.5f), polar (c, a, ringR + len * 0.5f)), 1.0f);
        }
    }

    // Fundamental marker: a small ivory tick at the pitch's position on the dial.
    if (f.life > 0.02f || smooth.numVisualNodes > 0)
    {
        const float a = -kPi * 0.5f + f.pitchU * kTwoPi;
        const float d = (3.0f + 2.0f * f.energy) * f.px;
        g.setColour (alpha (Theme::ivory, 0.25f + 0.55f * f.life));
        g.drawLine (juce::Line<float> (polar (c, a, ringR - d), polar (c, a, ringR + d)), juce::jmax (1.0f, 1.2f * f.px));
    }

    for (int i = 0; i < smooth.numVisualNodes && i < VisualStateSnapshot::kVisualNodes; ++i)
    {
        const float hz = smooth.nodeFrequency[i];
        if (hz <= 0.0f) continue;
        const float u = sat (std::log2 (hz / 30.0f) / 9.0f);
        const float a = -kPi * 0.5f + u * kTwoPi;
        const float r = ringR + 0.09f * f.R * smooth.nodePan[i];
        const float e = juce::jmax (sat (smooth.nodeEnergy[i] * 6.0f), 0.3f * f.energy * f.life);
        const float s = (1.4f + 3.2f * e) * f.px * (1.0f + 0.3f * f.pulse);
        const auto col = clusterColour (smooth.nodeCluster[i]);
        const auto p = polar (c, a, r);
        // spoke toward the object: the node's energy feeding the body
        g.setColour (alpha (col, 0.05f + 0.25f * e));
        g.drawLine (juce::Line<float> (polar (c, a, ringR - f.R * 0.05f), polar (c, a, ringR - f.R * (0.12f + 0.1f * e))), juce::jmax (0.7f, 0.9f * f.px));
        draw::glowEllipse (g, juce::Rectangle<float> (p.x - s, p.y - s, s * 2.0f, s * 2.0f), col, s * 2.5f, 0.4f + 0.6f * e);
        g.setColour (alpha (col.interpolatedWith (Theme::ivory, 0.4f * e), 0.35f + 0.65f * e));
        g.fillEllipse (p.x - s * 0.5f, p.y - s * 0.5f, s, s);
    }
}

void AntiMatterVisualizer::drawCaptions (juce::Graphics& g, juce::Rectangle<float> area)
{
    if (area.getHeight() < 200.0f) return;
    const float h = juce::jlimit (7.5f, 10.0f, area.getHeight() * 0.02f);
    auto tl = area.removeFromTop (30.0f);
    draw::trackedText (g, "INHALE", tl.withWidth (90.0f).withTrimmedLeft (8.0f), juce::Justification::topLeft, Theme::captionFont (h), Theme::textPrimary.withAlpha (0.8f));
    draw::trackedText (g, "IDEA", tl.withWidth (90.0f).withTrimmedLeft (8.0f).withTrimmedTop (h * 1.5f), juce::Justification::topLeft, Theme::captionFont (h - 1.0f), Theme::textDim);
    draw::trackedText (g, "EXHALE", tl.withLeft (tl.getRight() - 90.0f).withTrimmedRight (8.0f), juce::Justification::topRight, Theme::captionFont (h), Theme::textPrimary.withAlpha (0.8f));
    draw::trackedText (g, "EVOLVE", tl.withLeft (tl.getRight() - 90.0f).withTrimmedRight (8.0f).withTrimmedTop (h * 1.5f), juce::Justification::topRight, Theme::captionFont (h - 1.0f), Theme::textDim);

    auto bottom = area.removeFromBottom (44.0f);
    const float titleH = juce::jlimit (12.0f, 22.0f, area.getWidth() * 0.04f);
    draw::trackedText (g, "ANTI-MATR", bottom.removeFromTop (titleH * 1.3f), juce::Justification::centred, Theme::titleFont (titleH), Theme::textPrimary.withAlpha (0.9f));
    draw::trackedText (g, "SOUND BEYOND MATTER", bottom, juce::Justification::centredTop, Theme::captionFont (h), Theme::textSecondary);
}

} // namespace am::ui
