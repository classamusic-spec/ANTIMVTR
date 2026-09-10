#include "AntiMatterVisualizer.h"
#include <cstdio>
#include <cstdlib>

namespace am::ui
{

namespace
{
    constexpr float kPi    = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
    constexpr float kFocal = 2.5f;        // camera distance in sphere radii

    inline juce::Colour alpha (juce::Colour c, float a) noexcept { return c.withAlpha (juce::jlimit (0.0f, 1.0f, a)); }
    inline float sat (float v) noexcept { return juce::jlimit (0.0f, 1.0f, v); }
    inline juce::Point<float> polar (juce::Point<float> c, float angle, float r) noexcept
    {
        return { c.x + std::cos (angle) * r, c.y + std::sin (angle) * r };
    }

    /** Per-quality-level caps. Level 0 = full, 1 = reduced, 2 = minimal. */
    struct QualityCaps
    {
        int  ribbons, samples, sparkles, bubbles;
        bool shadows, bloom, smudges;
    };
    constexpr QualityCaps kCaps[3] = {
        { 20, 42, 120, 16, true,  true,  true  },
        { 16, 30,  70, 10, false, true,  false },
        { 10, 22,  32,  5, false, false, false },
    };

    /** Environmental tint per Space type: NEBULA|VOID|CHAMBER|ORBIT|DREAM|MACHINE|SHIMMER|DUST */
    juce::Colour spaceTint (int type) noexcept
    {
        static const juce::Colour tints[8] = {
            Theme::violet, juce::Colour (0xff232342), Theme::amber, Theme::blue,
            Theme::magenta, Theme::cyan, Theme::ivory, juce::Colour (0xffa8957f),
        };
        return tints[juce::jlimit (0, 7, type)];
    }
}

//==============================================================================
AntiMatterVisualizer::AntiMatterVisualizer (Diagnostics& d)
    : diag (d), noise (0x5EEDA11u), organism (0x11B0Fu), particles (0xA11CEu)
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
    profileToStderr = std::getenv ("ANTIMATR_VIS_PROFILE") != nullptr;
    if (const char* pin = std::getenv ("ANTIMATR_VIS_QUALITY"))
        quality = pinnedQuality = juce::jlimit (0, 2, std::atoi (pin));

    ribbonPath.preallocateSpace (LiquidOrganism::kMaxSamples * 6 + 16);
    corePath.preallocateSpace (256);
    glassPath.preallocateSpace (32);
    scratchPath.preallocateSpace (128);
    gradient.clearColours();

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
    smooth.numVisualNodes = latest.numVisualNodes;
    for (int i = 0; i < latest.numVisualNodes && i < VisualStateSnapshot::kVisualNodes; ++i)
    {
        smooth.nodeFrequency[i] = latest.nodeFrequency[i];
        easeTo (smooth.nodeEnergy[i], latest.nodeEnergy[i], 18.0f);
        smooth.nodePan[i] = latest.nodePan[i];
        smooth.nodeCluster[i] = latest.nodeCluster[i];
    }

    // Fracture: sparkle bursts only while the stage is on.
    easeTo (fracture, latest.fractureOn ? sat (latest.fractureActivity * 1.5f) : 0.0f, latest.fractureOn ? 6.0f : 2.5f);

    // Audio pulse: fast attack, release governed by Decay.
    const float level = sat (latest.rmsL * 4.5f + latest.peak * 0.5f);
    if (level > pulse) easeTo (pulse, level, 30.0f);
    else               easeTo (pulse, level, 12.0f - 10.0f * smooth.decay);

    easeTo (energy, sat (latest.noteEnergy), latest.noteEnergy > energy ? 14.0f : 4.0f - 3.0f * smooth.decay);
    easeTo (life, latest.activeVoices > 0 ? 1.0f : 0.0f, latest.activeVoices > 0 ? 4.0f : 0.8f);

    // FREEZE stops the flow dead: the flow clock and the rotation stop advancing,
    // eased over a fifth of a second so it reads as the fluid seizing, not a jump cut.
    easeTo (freezeMix, latest.freeze ? 1.0f : 0.0f, 6.0f);
    const float run = 1.0f - freezeMix;

    flowTime += dt * run * (0.16f + 0.30f * smooth.decay + 0.35f * life + 0.9f * fracture);
    rotation += dt * run * (0.048f + 0.075f * (1.0f - smooth.mass)) * (0.45f + 0.55f * life) * (1.0f + 1.6f * fracture);
    if (rotation > kTwoPi) rotation -= kTwoPi;
    hueDrift += dt * run * (0.010f + 0.026f * life + 0.05f * fracture);
    if (hueDrift > 1.0f) hueDrift -= 1.0f;

    // Particles advance in object units; painting converts to pixels.
    computeBudget();
    ParticleSystem::Env env;
    env.dt = dt; env.time = time; env.flowTime = flowTime;
    env.density = smooth.density; env.decay = smooth.decay; env.tension = smooth.tension;
    env.melt = smooth.melt; env.scatter = smooth.scatter;
    env.life = life; env.level = pulse; env.fracture = fracture;
    env.breathe = 1.0f;
    sparklesAlive = ParticleSystem::sparkleCount (env, capSparkles);
    bubblesAlive = ParticleSystem::bubbleCount (env, capBubbles);
    particles.updateSparkles (env, noise, sparklesAlive);
    particles.updateBubbles (env, noise, bubblesAlive);
}

/**
    Detail scales with the porthole so the minimum editor size stays smooth:
    DENSITY still drives the look, but a small porthole caps how much is drawn.
*/
void AntiMatterVisualizer::computeBudget()
{
    const auto& caps = kCaps[juce::jlimit (0, 2, quality)];
    const auto port = PortholeLayout::forBounds (0.0f, 0.0f, (float) getWidth(), (float) getHeight());
    const float detail = juce::jlimit (0.50f, 1.0f, port.outerR / 175.0f);

    capRibbons  = juce::jmax (6, (int) ((float) caps.ribbons * detail));
    capSamples  = juce::jmax (10, (int) ((float) caps.samples * (0.55f + 0.45f * detail)));
    capSparkles = juce::jmax (8, (int) ((float) caps.sparkles * detail));
    capBubbles  = juce::jmax (3, (int) ((float) caps.bubbles * detail));
    capShadows  = caps.shadows;
    capBloom    = caps.bloom;
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
        static const char* names[kLayers] = { "well", "farParticles", "backRibbons", "core", "frontRibbons",
                                              "nearParticles", "glass", "bezel", "plinth", "captions", "clip" };
        const auto port = PortholeLayout::forBounds (0.0f, 0.0f, (float) getWidth(), (float) getHeight());
        std::fprintf (stderr, "[antimatter] paint %.2f ms avg, quality %d, %dx%d, ribbons %d, spans %d, extent %.2f\n",
                      (double) paintMsAverage, quality, getWidth(), getHeight(), numRibbons, numSpans,
                      (double) (objectExtent / juce::jmax (1.0f, port.outerR)));
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
    f.surfaceRough = sat (smooth.surface);

    f.maxRibbons = capRibbons; f.maxSamples = capSamples;
    f.maxSparkles = capSparkles; f.maxBubbles = capBubbles;
    f.shadows = capShadows; f.bloom = capBloom; f.smudges = capSmudges;

    // Size and brightness follow level; the object breathes so it never looks asleep,
    // except under FREEZE, where the idle breath stops with the rest of the flow.
    f.breathe = 1.0f + 0.018f * std::sin (time * 0.7f) * (1.0f - freezeMix)
                + 0.055f * f.pulse + 0.025f * f.energy * f.life;
    f.R = f.port.glassR * (0.775f + 0.030f * smooth.mass + 0.030f * f.pulse);

    f.p.time = time;
    f.p.flowTime = flowTime;
    f.p.rotation = rotation;
    f.p.pitch = 0.34f;
    f.p.density = smooth.density; f.p.form = smooth.form; f.p.mass = smooth.mass;
    f.p.tension = smooth.tension; f.p.decay = smooth.decay; f.p.surface = smooth.surface;
    f.p.bend = smooth.bend; f.p.melt = smooth.melt; f.p.tear = smooth.tear;
    f.p.magnet = smooth.magnet; f.p.gravity = smooth.gravity; f.p.scatter = smooth.scatter;
    f.p.crush = smooth.crush;
    f.p.level = f.pulse; f.p.energy = f.energy; f.p.life = f.life; f.p.fracture = f.fracture;
    f.p.breathe = f.breathe;
    f.coreR = LiquidOrganism::coreRadius (f.p);

    f.cosYaw = std::cos (rotation);   f.sinYaw = std::sin (rotation);
    f.cosPitch = std::cos (f.p.pitch); f.sinPitch = std::sin (f.p.pitch);

    refreshHardware (f, juce::jmax (0.5f, g.getInternalContext().getPhysicalPixelScaleFactor()));
    buildOrganism (f);

    // ---- 1. the glow spilling from beneath the plinth (behind everything)
    drawPlinthGlow (g, f);                            mark (8);

    // ---- 2/3. the glass well and the object, clipped to the glass.
    //      Building an edge table from the circle costs about a sixth of the frame,
    //      so it is only done when the object actually reaches past the bezel —
    //      normally a plain rectangle is enough and nothing can escape it.
    {
        juce::Graphics::ScopedSaveState clip (g);
        if (objectExtent > f.port.outerR)
        {
            glassPath.clear();
            glassPath.addEllipse (f.centre.x - f.port.glassR, f.centre.y - f.port.glassR,
                                  f.port.glassR * 2.0f, f.port.glassR * 2.0f);
            g.reduceClipRegion (glassPath);
        }
        else
        {
            g.reduceClipRegion (juce::Rectangle<float> (f.centre.x - f.port.glassR, f.centre.y - f.port.glassR,
                                                        f.port.glassR * 2.0f, f.port.glassR * 2.0f)
                                    .getSmallestIntegerContainer());
        }
        mark (10);
        drawWell (g, f);                              mark (0);
        drawBubbles (g, f, false);
        drawSparkles (g, f, false);                   mark (1);
        drawSpanRange (g, f, 0, firstFrontSpan);      mark (2);
        drawCore (g, f);                              mark (3);
        drawSpanRange (g, f, firstFrontSpan, numSpans); mark (4);
        drawSparkles (g, f, true);
        drawBubbles (g, f, true);                     mark (5);
        if (f.smudges) drawSmudges (g, f);
    }

    // ---- 4. the glass, composited over the object
    blit (g, glassImage, glassArea);
    mark (6);

    // ---- 5/6. the bezel and its status lamps
    blit (g, bezelImage, bezelArea);
    drawLamps (g, f);
    mark (7);

    // ---- 7. the plinth
    blit (g, plinthImage, plinthArea);
    mark (8);

    // ---- 8. flank captions
    drawCaptions (g, f);
    mark (9);

    const double ms = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1000.0;
    paintMsAverage = paintMsAverage <= 0.0f ? (float) ms : paintMsAverage + ((float) ms - paintMsAverage) * 0.08f;
    for (int i = 0; i < kLayers; ++i)
        layerMsAverage[(size_t) i] += (frameLayerMs[(size_t) i] - layerMsAverage[(size_t) i]) * 0.08;
}

//==============================================================================
void AntiMatterVisualizer::buildOrganism (const Frame& f)
{
    numRibbons = juce::jmin (LiquidOrganism::ribbonCount (f.p), f.maxRibbons);
    const int maxSamples = f.maxSamples;
    numSpans = 0;
    objectExtent = f.coreR * f.R * f.breathe * 1.6f;

    // Node energies let the resonant matter itself brighten individual ribbons.
    const int nodes = juce::jlimit (0, VisualStateSnapshot::kVisualNodes, smooth.numVisualNodes);

    for (int r = 0; r < numRibbons; ++r)
    {
        auto& row = samples[(size_t) r];
        const int n = organism.buildRibbon (r, f.p, noise, row.data(), maxSamples);
        sampleLength[(size_t) r] = n;
        if (n < 2) continue;

        const float nodeBoost = nodes > 0 ? sat (smooth.nodeEnergy[r % nodes] * 5.0f) : 0.0f;

        // Project: yaw, then a fixed pitch, then a perspective divide.
        auto& out = screen[(size_t) r];
        for (int i = 0; i < n; ++i)
        {
            const Vec3& q = row[(size_t) i].p;
            const float x1 = q.x * f.cosYaw + q.z * f.sinYaw;
            const float z1 = -q.x * f.sinYaw + q.z * f.cosYaw;
            const float y2 = q.y * f.cosPitch - z1 * f.sinPitch;
            const float z2 = q.y * f.sinPitch + z1 * f.cosPitch;
            const float s = kFocal / juce::jmax (0.6f, kFocal - z2);
            out[(size_t) i].x = f.centre.x + x1 * f.R * s;
            out[(size_t) i].y = f.centre.y + y2 * f.R * s;
            out[(size_t) i].z = juce::jlimit (-1.6f, 1.6f, z2);
            out[(size_t) i].scale = s;
            row[(size_t) i].bright = sat (row[(size_t) i].bright + nodeBoost * 0.35f);
        }

        // Screen-space normals and per-edge half widths (SURFACE roughens the two edges apart).
        for (int i = 0; i < n; ++i)
        {
            const int a = i > 0 ? i - 1 : 0, b = i < n - 1 ? i + 1 : n - 1;
            float tx = out[(size_t) b].x - out[(size_t) a].x;
            float ty = out[(size_t) b].y - out[(size_t) a].y;
            const float len = std::sqrt (tx * tx + ty * ty);
            if (len > 1.0e-4f) { tx /= len; ty /= len; } else { tx = 1.0f; ty = 0.0f; }
            out[(size_t) i].nx = -ty;
            out[(size_t) i].ny = tx;

            // A ribbon may never be wider than the radius of its own turn, or the
            // strip folds back through itself and shows a bow tie at the cusp.
            const float dPrev = std::hypot (out[(size_t) i].x - out[(size_t) a].x, out[(size_t) i].y - out[(size_t) a].y);
            const float dNext = std::hypot (out[(size_t) b].x - out[(size_t) i].x, out[(size_t) b].y - out[(size_t) i].y);
            const float step = juce::jmax (0.6f, 0.5f * (dPrev + dNext));
            float turn = 0.0f;
            if (i > 0 && i < n - 1)
            {
                const float ax = out[(size_t) i].x - out[(size_t) (i - 1)].x, ay = out[(size_t) i].y - out[(size_t) (i - 1)].y;
                const float bx = out[(size_t) (i + 1)].x - out[(size_t) i].x, by = out[(size_t) (i + 1)].y - out[(size_t) i].y;
                const float la = std::sqrt (ax * ax + ay * ay), lb = std::sqrt (bx * bx + by * by);
                if (la > 1.0e-4f && lb > 1.0e-4f)
                    turn = std::acos (juce::jlimit (-1.0f, 1.0f, (ax * bx + ay * by) / (la * lb)));
            }
            const float curveLimit = 1.05f * step / juce::jmax (0.09f, turn);
            // Depth thins the ribbon: the far side of the sphere is not just dimmer,
            // it is narrower, and that is half of what makes the mass read as solid.
            const float depthWidth = 0.52f + 0.48f * sat (0.5f + 0.5f * out[(size_t) i].z);
            const float base = juce::jmin (row[(size_t) i].width * f.R * out[(size_t) i].scale * depthWidth, curveLimit);
            if (f.surfaceRough > 0.02f)
            {
                const float ft = flowTime * 1.6f;
                const float s0 = noise.noise ((float) i * 1.7f + (float) r * 17.0f, ft * 1.1f)
                                 + 0.5f * noise.noise ((float) i * 4.1f + (float) r * 7.0f, ft * 1.9f);
                const float s1 = noise.noise ((float) i * 1.7f + (float) r * 17.0f + 53.0f, ft * 1.3f)
                                 + 0.5f * noise.noise ((float) i * 4.1f + (float) r * 7.0f + 29.0f, ft * 2.2f);
                out[(size_t) i].w0 = juce::jmax (0.0f, base * (1.0f + f.surfaceRough * 0.60f * s0));
                out[(size_t) i].w1 = juce::jmax (0.0f, base * (1.0f + f.surfaceRough * 0.60f * s1));
            }
            else
            {
                out[(size_t) i].w0 = base;
                out[(size_t) i].w1 = base;
            }

            // The glass drinks the light at its edge: a ribbon narrows and dims as it
            // approaches the rim, so the organism dissolves into the dark instead of
            // being cut off — and nothing the object draws can ever reach the bezel.
            const float dist = std::hypot (out[(size_t) i].x - f.centre.x, out[(size_t) i].y - f.centre.y);
            const float fade = 1.0f - liquid::smoothstep (f.port.glassR * 0.845f, f.port.glassR * 0.965f, dist);
            out[(size_t) i].w0 *= fade;
            out[(size_t) i].w1 *= fade;
            row[(size_t) i].bright *= 0.10f + 0.90f * fade;

            // 2.1 = the widest strip (1.66) times the largest depth width multiplier.
            const float reach = dist + 2.1f * juce::jmax (out[(size_t) i].w0, out[(size_t) i].w1);
            if (reach > objectExtent) objectExtent = reach;
        }

        numSpans += LiquidOrganism::splitByDepth (r, row.data(), n, organism.isAngular (f.p), f.coreR * 1.02f,
                                                  spans.data() + numSpans, LiquidOrganism::kMaxSpans - numSpans);
    }

    LiquidOrganism::sortByDepth (spans.data(), numSpans);
    firstFrontSpan = 0;
    while (firstFrontSpan < numSpans && spans[(size_t) firstFrontSpan].meanZ < 0.0f) ++firstFrontSpan;
}

//==============================================================================
void AntiMatterVisualizer::renderWell (juce::Graphics& g, const Frame& f)
{
    // The deep bowl the object is suspended in, with the Space-tinted haze that
    // fills it. Static, so it is cached and re-rendered only on a resize or a
    // change of Space: radial gradients this large are the most expensive fill
    // in the frame and there is no budget to repeat them at 30 Hz.
    const float Rg = f.port.glassR;
    const auto c = f.centre;

    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = { c.x - Rg * 0.18f, c.y - Rg * 0.22f };
    gradient.point2 = { c.x - Rg * 0.18f + Rg * 1.28f, c.y - Rg * 0.22f };
    gradient.addColour (0.0, juce::Colour (0xff0b0c1a));
    gradient.addColour (0.55, juce::Colour (0xff05050e));
    gradient.addColour (1.0, juce::Colour (0xff010104));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - Rg, c.y - Rg, Rg * 2.0f, Rg * 2.0f);

    const float hr = Rg * 0.86f;
    const auto glow = Theme::violet.interpolatedWith (Theme::blue, 0.35f).interpolatedWith (f.space, 0.34f);
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = c;
    gradient.point2 = { c.x + hr, c.y };
    gradient.addColour (0.0, alpha (glow, 0.17f));
    gradient.addColour (0.42, alpha (glow, 0.085f));
    gradient.addColour (1.0, alpha (glow, 0.0f));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);
}

void AntiMatterVisualizer::drawWell (juce::Graphics& g, const Frame& f)
{
    blit (g, wellImage, wellArea);

    // The one live light in the well: the object's own bloom, answering the level.
    // Kept small — a full-glass radial gradient costs more than the object does.
    const auto c = f.centre;
    const float br = f.R * (0.62f + 0.22f * f.pulse + 0.16f * f.spaceActivity);
    const auto glow = Theme::violet.interpolatedWith (Theme::cyan, 0.22f + 0.30f * f.pulse);
    const float a = 0.10f + 0.20f * f.pulse + 0.07f * f.life + 0.06f * f.energy;
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = c;
    gradient.point2 = { c.x + br, c.y };
    gradient.addColour (0.0, alpha (glow, a));
    gradient.addColour (0.45, alpha (glow, a * 0.5f));
    gradient.addColour (1.0, alpha (glow, 0.0f));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
}

//==============================================================================
void AntiMatterVisualizer::buildSpanPath (juce::Path& path, const RibbonSpan& span, float widthScale,
                                          float offsetX, float offsetY) const
{
    const auto& sc = screen[(size_t) span.ribbon];
    const int a = span.first, b = span.first + span.count - 1;

    auto edge = [&] (int i, float side)
    {
        const auto& q = sc[(size_t) i];
        const float w = (side > 0.0f ? q.w0 : q.w1) * widthScale * side;
        return juce::Point<float> (q.x + q.nx * w + offsetX, q.y + q.ny * w + offsetY);
    };
    auto midway = [] (juce::Point<float> p, juce::Point<float> q)
    {
        return juce::Point<float> ((p.x + q.x) * 0.5f, (p.y + q.y) * 0.5f);
    };

    // A quadratic B-spline through the sample points: the silhouette of a ribbon
    // of liquid must never show the straight segments it is sampled from. Under
    // CRUSH the fluid has gone angular, and then the corners are the point.
    path.clear();
    path.startNewSubPath (edge (a, 1.0f));
    if (span.angular)
    {
        for (int i = a + 1; i <= b; ++i) path.lineTo (edge (i, 1.0f));
        for (int i = b; i >= a; --i) path.lineTo (edge (i, -1.0f));
    }
    else
    {
        for (int i = a + 1; i < b; ++i)
            path.quadraticTo (edge (i, 1.0f), midway (edge (i, 1.0f), edge (i + 1, 1.0f)));
        path.lineTo (edge (b, 1.0f));
        path.lineTo (edge (b, -1.0f));
        for (int i = b - 1; i > a; --i)
            path.quadraticTo (edge (i, -1.0f), midway (edge (i, -1.0f), edge (i - 1, -1.0f)));
        path.lineTo (edge (a, -1.0f));
    }
    path.closeSubPath();
}

void AntiMatterVisualizer::drawRibbonSpan (juce::Graphics& g, const Frame& f, const RibbonSpan& span)
{
    const auto& sc = screen[(size_t) span.ribbon];
    const auto& sm = samples[(size_t) span.ribbon];
    const int a = span.first, b = span.first + span.count - 1;
    const int mid = (a + b) / 2;

    // Depth, per end of the span rather than per span: a ribbon that arcs from the
    // back of the sphere to the front has to darken and pale along its own length,
    // or the whole mass shades flat however carefully the runs are sorted.
    auto depthOf = [&] (int i) { return juce::jlimit (-1.0f, 1.0f, sc[(size_t) i].z / 1.15f); };
    const float dA = depthOf (a), dM = depthOf (mid), dB = depthOf (b);
    const float meanDepth = juce::jlimit (-1.0f, 1.0f, span.meanZ / 1.15f);
    const bool  back = meanDepth < 0.0f;

    // 0.24 at the far side of the sphere, 1.0 at the glass. The floor is high enough
    // that a ribbon at the silhouette still reads once the editor is seen whole.
    auto dim = [] (float d) { const float t = 0.5f + 0.5f * d; return 0.24f + 0.76f * t * std::sqrt (t); };
    const float dimA = dim (dA), dimM = dim (dM), dimB = dim (dB);

    const auto pa = juce::Point<float> (sc[(size_t) a].x, sc[(size_t) a].y);
    const auto pb = juce::Point<float> (sc[(size_t) b].x, sc[(size_t) b].y);
    const bool degenerate = pa.getDistanceFrom (pb) < 2.0f;

    const float hueA = organism.ribbonHue (span.ribbon, sm[(size_t) a].u, f.hue);
    const float hueM = organism.ribbonHue (span.ribbon, sm[(size_t) mid].u, f.hue);
    const float hueB = organism.ribbonHue (span.ribbon, sm[(size_t) b].u, f.hue);
    const float brA = sm[(size_t) a].bright, brM = sm[(size_t) mid].bright, brB = sm[(size_t) b].bright;

    // A ribbon at the back loses its colour into the cold volume; one in front keeps it.
    auto lay = [&] (float wScale, float base, float whiten)
    {
        buildSpanPath (ribbonPath, span, wScale, 0.0f, 0.0f);
        if (degenerate)
        {
            g.setColour (alpha (iridescence.depthShade (hueM, dM, whiten * (0.35f + 0.65f * dimM)), base * 1.25f * dimM * brM));
            g.fillPath (ribbonPath);
            return;
        }
        gradient.clearColours();
        gradient.isRadial = false;
        gradient.point1 = pa;
        gradient.point2 = pb;
        gradient.addColour (0.0, alpha (iridescence.depthShade (hueA, dA, whiten * (0.35f + 0.65f * dimA)), base * dimA * brA));
        gradient.addColour (0.5, alpha (iridescence.depthShade (hueM, dM, whiten * (0.35f + 0.65f * dimM)), base * 1.25f * dimM * brM));
        gradient.addColour (1.0, alpha (iridescence.depthShade (hueB, dB, whiten * (0.35f + 0.65f * dimB)), base * dimB * brB));
        g.setGradientFill (gradient);
        g.fillPath (ribbonPath);
    };

    // A ribbon in front casts a faint shadow onto whatever is behind it.
    if (f.shadows && meanDepth > 0.30f)
    {
        const float d = f.unit * (2.4f + 4.0f * meanDepth);
        buildSpanPath (ribbonPath, span, 1.30f, d, d * 0.85f);
        g.setColour (alpha (juce::Colour (0xff02030a), 0.18f + 0.20f * meanDepth));
        g.fillPath (ribbonPath);
    }

    // Nested strips of falling width and rising alpha: a translucent bloom that
    // dissolves smoothly on both sides of a molten core. A single wide fill would
    // show a hard silhouette exactly where the light should be fading away, and a
    // single narrow one is a neon stroke.
    struct Strip { float width, alpha, whiten; };
    // Four nested strips: a translucent bloom that dissolves on both sides of a
    // molten core. The widest, faintest wash was dropped — at an alpha of 0.07 it
    // cost more of the frame than any other layer and returned almost nothing once
    // the editor is seen whole, and that budget buys brightness instead.
    static constexpr Strip kFront[4] = {
        { 1.66f, 0.105f, 0.00f }, { 1.04f, 0.290f, 0.02f }, { 0.56f, 0.600f, 0.14f }, { 0.14f, 1.000f, 0.64f },
    };
    static constexpr Strip kBack[3] = {
        { 1.62f, 0.090f, 0.00f }, { 1.06f, 0.215f, 0.00f }, { 0.46f, 0.460f, 0.12f },
    };
    const Strip* strips = back ? kBack : kFront;
    const int count = back ? 3 : 4;
    const int first = f.bloom ? 0 : 1;              // reduced quality drops the widest, softest layer

    for (int k = first; k < count; ++k)
        lay (strips[k].width, strips[k].alpha, strips[k].whiten);
}

void AntiMatterVisualizer::drawSpanRange (juce::Graphics& g, const Frame& f, int from, int to)
{
    for (int i = juce::jmax (0, from); i < juce::jmin (to, numSpans); ++i)
        drawRibbonSpan (g, f, spans[(size_t) i]);
}

//==============================================================================
void AntiMatterVisualizer::drawCore (juce::Graphics& g, const Frame& f)
{
    const auto c = f.centre;
    const float cr = f.coreR * f.R * f.breathe;
    if (cr < 1.0f) return;

    // The volume darkening around the mass: ribbons behind it sink into this.
    const float hr = cr * 1.62f;
    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = c;
    gradient.point2 = { c.x + hr, c.y };
    gradient.addColour (0.0, alpha (juce::Colour (0xff01010a), 0.95f));
    gradient.addColour (0.62, alpha (juce::Colour (0xff02020c), 0.72f));
    gradient.addColour (1.0, alpha (juce::Colour (0xff03031a), 0.0f));
    g.setGradientFill (gradient);
    g.fillEllipse (c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);

    // The silhouette: a slowly deforming lobed blob, never a circle.
    const int segs = juce::jlimit (28, 64, (int) (f.port.outerR * 0.22f));
    corePath.clear();
    for (int i = 0; i < segs; ++i)
    {
        const float ang = (float) i / (float) segs * kTwoPi;
        const float r = cr * organism.coreProfile (ang + rotation * 0.5f, f.p, noise);
        const auto p = polar (c, ang, r);
        if (i == 0) corePath.startNewSubPath (p); else corePath.lineTo (p);
    }
    corePath.closeSubPath();

    gradient.clearColours();
    gradient.isRadial = true;
    gradient.point1 = { c.x - cr * 0.35f, c.y - cr * 0.40f };
    gradient.point2 = { c.x - cr * 0.35f + cr * 1.7f, c.y - cr * 0.40f };
    gradient.addColour (0.0, juce::Colour (0xff17142e));
    gradient.addColour (0.42, juce::Colour (0xff090820));
    gradient.addColour (0.78, juce::Colour (0xff03030d));
    gradient.addColour (1.0, juce::Colour (0xff010105));
    g.setGradientFill (gradient);
    g.fillPath (corePath);

    // The faint violet rim where the light wraps around the mass: a soft halo just
    // outside the silhouette, then a fine bright line on it.
    const float rimW = juce::jmax (1.0f, f.unit * 1.7f);
    gradient.clearColours();
    gradient.isRadial = false;
    gradient.point1 = { c.x - cr, c.y - cr };
    gradient.point2 = { c.x + cr, c.y + cr };
    gradient.addColour (0.0, alpha (Theme::violet, 0.05f + 0.05f * f.pulse));
    gradient.addColour (0.42, alpha (Theme::violet, 0.13f + 0.10f * f.pulse));
    gradient.addColour (1.0, alpha (Theme::magenta, 0.18f + 0.10f * f.pulse));
    g.setGradientFill (gradient);
    g.strokePath (corePath, juce::PathStrokeType (rimW * 4.2f));

    gradient.clearColours();
    gradient.isRadial = false;
    gradient.point1 = { c.x - cr, c.y - cr };
    gradient.point2 = { c.x + cr, c.y + cr };
    gradient.addColour (0.0, alpha (Theme::violet, 0.10f + 0.10f * f.pulse));
    gradient.addColour (0.42, alpha (Theme::violet, 0.26f + 0.22f * f.pulse));
    gradient.addColour (1.0, alpha (Theme::magenta, 0.36f + 0.24f * f.pulse));
    g.setGradientFill (gradient);
    g.strokePath (corePath, juce::PathStrokeType (rimW * 1.05f));

    // MASS opens the core: an aperture of light widens inside the mass.
    const float open = juce::jlimit (0.0f, 1.0f, (smooth.mass - 0.42f) * 1.9f);
    if (open > 0.01f)
    {
        const float ar = cr * (0.14f + 0.44f * open);
        const auto lit = Theme::cyan.interpolatedWith (Theme::violet, 0.45f);
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = c;
        gradient.point2 = { c.x + ar, c.y };
        gradient.addColour (0.0, alpha (lit, (0.30f + 0.30f * f.pulse) * open));
        gradient.addColour (0.55, alpha (lit, 0.13f * open));
        gradient.addColour (1.0, alpha (lit, 0.0f));
        g.setGradientFill (gradient);
        g.fillEllipse (c.x - ar, c.y - ar, ar * 2.0f, ar * 2.0f);
    }
}

//==============================================================================
void AntiMatterVisualizer::drawSparkles (juce::Graphics& g, const Frame& f, bool front)
{
    const int count = sparklesAlive;
    const auto& list = particles.sparkleArray();
    const auto c = f.centre;

    for (int i = 0; i < count && i < ParticleSystem::kMaxSparkles; ++i)
    {
        const auto& s = list[(size_t) i];
        const float x1 = s.p.x * f.cosYaw + s.p.z * f.sinYaw;
        const float z1 = -s.p.x * f.sinYaw + s.p.z * f.cosYaw;
        const float y2 = s.p.y * f.cosPitch - z1 * f.sinPitch;
        const float z2 = s.p.y * f.sinPitch + z1 * f.cosPitch;
        if ((z2 >= 0.0f) != front) continue;

        const float scale = kFocal / juce::jmax (0.6f, kFocal - z2);
        const juce::Point<float> p (c.x + x1 * f.R * scale, c.y + y2 * f.R * scale);
        const float depthMix = sat (0.5f + 0.5f * z2);
        if (p.getDistanceFrom (c) > f.port.glassR * 0.985f) continue;
        const float d = juce::jmax (0.8f, s.size * f.unit * 2.0f * scale * (0.85f + 0.4f * f.pulse));
        const float a = s.bright * (0.34f + 0.66f * depthMix) * (0.70f + 0.30f * f.life);
        if (a < 0.02f) continue;

        if (front && s.bright > 0.62f && d > 1.3f)
        {
            const float hr = d * 2.6f;
            g.setColour (alpha (iridescence.at (s.hue + f.hue), 0.14f * a));
            g.fillEllipse (p.x - hr, p.y - hr, hr * 2.0f, hr * 2.0f);
        }
        g.setColour (alpha (iridescence.lit (s.hue + f.hue, 0.55f + 0.35f * depthMix), a));
        g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
    }
}

void AntiMatterVisualizer::drawBubbles (juce::Graphics& g, const Frame& f, bool front)
{
    const int count = bubblesAlive;
    const auto& list = particles.bubbleArray();
    const auto c = f.centre;

    for (int i = 0; i < count && i < ParticleSystem::kMaxBubbles; ++i)
    {
        const auto& bb = list[(size_t) i];
        const float x1 = bb.p.x * f.cosYaw + bb.p.z * f.sinYaw;
        const float z1 = -bb.p.x * f.sinYaw + bb.p.z * f.cosYaw;
        const float y2 = bb.p.y * f.cosPitch - z1 * f.sinPitch;
        const float z2 = bb.p.y * f.sinPitch + z1 * f.cosPitch;
        if ((z2 >= 0.0f) != front) continue;

        const float scale = kFocal / juce::jmax (0.6f, kFocal - z2);
        const juce::Point<float> p (c.x + x1 * f.R * scale, c.y + y2 * f.R * scale);
        const float depthMix = sat (0.5f + 0.5f * z2);
        // Nearer bubbles are larger and softer; far ones are small and faint.
        const float r = bb.radius * f.R * scale * (0.55f + 0.70f * depthMix) * (1.0f + 0.15f * f.pulse);
        if (r < 1.2f || p.getDistanceFrom (c) + r > f.port.glassR * 0.995f) continue;
        const float a = (0.18f + 0.36f * depthMix) * (0.62f + 0.38f * f.life);
        const auto col = iridescence.at (bb.hue + f.hue * 0.6f);

        // Body: hollow, brightest where the wall is edge-on. Softer the nearer it is.
        const float wall = juce::jlimit (0.55f, 0.90f, 0.86f - 0.30f * depthMix);
        gradient.clearColours();
        gradient.isRadial = true;
        gradient.point1 = p;
        gradient.point2 = { p.x + r, p.y };
        gradient.addColour (0.0, alpha (col, 0.05f * a));
        gradient.addColour ((double) wall, alpha (col, 0.22f * a));
        gradient.addColour (0.96, alpha (col, 0.85f * a));
        gradient.addColour (1.0, alpha (col, 0.0f));
        g.setGradientFill (gradient);
        g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);

        // Rim highlight where the light wraps the lower right.
        scratchPath.clear();
        scratchPath.addCentredArc (p.x, p.y, r * 0.90f, r * 0.90f, 0.0f, 0.45f, 2.55f, true);
        g.setColour (alpha (Theme::ivory, 0.42f * a));
        g.strokePath (scratchPath, juce::PathStrokeType (juce::jmax (0.8f, r * 0.13f),
                                                        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Specular dot from the key light, top-left.
        const float sd = juce::jmax (1.2f, r * 0.30f);
        g.setColour (alpha (Theme::ivory, juce::jlimit (0.0f, 1.0f, (0.65f + 0.35f * depthMix) * a * 2.1f)));
        g.fillEllipse (p.x - r * 0.44f - sd * 0.5f, p.y - r * 0.46f - sd * 0.5f, sd, sd);
    }
}

} // namespace am::ui
