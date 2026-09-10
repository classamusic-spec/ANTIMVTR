#include "GestureSource.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

using excitation::softLimit;
using excitation::expMap;

namespace
{
    /**
        PolyBLEP residual for a unit step at fractional distance `t` (in cycles)
        from a discontinuity, with `dt` the phase increment per sample. Adding
        `height * polyBlep(...)` to a naive edge band limits it.
    */
    inline float polyBlep (double t, double dt) noexcept
    {
        if (dt <= 0.0) return 0.0f;
        if (t < dt)             { const double x = t / dt;         return (float) (x + x - x * x - 1.0); }
        if (t > 1.0 - dt)       { const double x = (t - 1.0) / dt; return (float) (x * x + x + x + 1.0); }
        return 0.0f;
    }

    inline float onePoleCoeff (float hz, double sr) noexcept
    {
        const float w = juce::jlimit (1.0e-5f, 0.49f, hz / (float) sr);
        return 1.0f - std::exp (-2.0f * (float) kPi * w);
    }

    /** One-pole coefficient for an exponential approach with time constant `seconds`. */
    inline float timeCoeff (float seconds, double sr) noexcept
    {
        return juce::jlimit (1.0e-5f, 1.0f, 1.0f - std::exp (-1.0f / juce::jmax (1.0f, seconds * (float) sr)));
    }
}

//==============================================================================
void GestureSource::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);
    sr = juce::jmax (8000.0, sampleRate);

    excitation::warmTables();
    tilt.prepare (sr);
    band.prepare (sr);
    formant1.prepare (sr);
    formant2.prepare (sr);
    tone.prepare (sr);
    pulseLp.prepare (sr);
    breathLp.prepare (sr);
    pulseDc.prepare (sr, 60.0f);
    dcL.prepare (sr, 12.0f);
    dcR.prepare (sr, 12.0f);

    attackCoeff  = timeCoeff (0.006f, sr);   // 6 ms in
    releaseCoeff = timeCoeff (0.030f, sr);   // 30 ms out

    formant1.set (760.0f, 3.2f);
    formant2.set (1850.0f, 4.5f);
    reset();
}

void GestureSource::reset()
{
    env = 0.0f;
    gate = false;
    pressureEff = p.pressure;
    speedEff = p.speed;
    motionA = motionB = motionTargetA = motionTargetB = 0.0f;
    motionHold = 0;
    controlCountdown = 0;
    bowPhase = 0.0; bowInc = 0.0; bowLp = 0.0f; bowGrip = 0.6f; bowSlipNoise = 0.0f;
    for (auto& g : grains) g = Grain();
    eventCountdown = 0.0;
    grainSum = 0.0f;
    rubPhase = 0.0f;
    turbulence = 0.5f; turbulenceTarget = 0.5f; turbulenceHold = 0;
    pulseCountdown = 0.0;
    sputter = 1.0f;
    comb.fill (0.0f);
    combWrite = 0; combDelay = 1; combDepth = 0.0f;
    pink.reset(); tilt.reset(); band.reset(); formant1.reset(); formant2.reset(); tone.reset();
    pulseLp.reset(); breathLp.reset(); pulseDc.reset(); dcL.reset(); dcR.reset();
    lastEnergy = 0.0f;
}

void GestureSource::noteOn (const NoteState& note, const ParamValues& params)
{
    juce::ignoreUnused (params);
    gate = true;
    noteId = note.noteId;

    // Deterministic per note: the same note id always draws the same gesture.
    noiseRng.reseed (hashSeed (noteId, 0x6E5715u));
    eventRng.reseed (hashSeed (noteId, 0x11FA37u));
    shapeRng.reseed (hashSeed (noteId, 0x2C0FFEu));

    bowPhase = 0.0;
    bowLp = 0.0f;
    eventCountdown = 0.0;
    pulseCountdown = 0.0;
    rubPhase = 0.0f;
    grainSum = 0.0f;
    controlCountdown = 0;
    for (auto& g : grains) g.active = false;
}

void GestureSource::noteOff()
{
    gate = false;
}

//==============================================================================
void GestureSource::readParams (const RenderContext& ctx, const NoteState& note)
{
    p.mode      = (Mode) juce::jlimit (0, (int) Mode::Count - 1, ctx.choice (Param::gestureMode));
    p.level     = clamp01 (ctx.param (Param::gestureLevel));
    p.pressure  = clamp01 (ctx.param (Param::gesturePressure));
    p.speed     = clamp01 (ctx.param (Param::gestureSpeed));
    p.roughness = clamp01 (ctx.param (Param::gestureRoughness));
    p.position  = clamp01 (ctx.param (Param::gesturePosition));
    p.motion    = clamp01 (ctx.param (Param::gestureMotion));
    p.bandwidth = clamp01 (ctx.param (Param::gestureBandwidth));
    p.velocity  = clamp01 (note.velocity);

    const double f = std::isfinite (note.frequency) ? note.frequency : (double) kMinFrequencyHz;
    p.freq = juce::jlimit ((double) kMinFrequencyHz, sr * 0.25, f);
}

void GestureSource::updateControl()
{
    // Slow seeded drift of pressure and speed. Two decorrelated random walks.
    if (--motionHold <= 0)
    {
        motionTargetA = shapeRng.nextBipolar();
        motionTargetB = shapeRng.nextBipolar();
        motionHold = (int) (sr * (0.12 + 0.5 * (double) shapeRng.nextFloat()) / (double) kControlSamples);
        motionHold = juce::jmax (1, motionHold);
    }
    const float follow = 0.02f + 0.12f * p.motion;
    motionA += follow * (motionTargetA - motionA);
    motionB += follow * (motionTargetB - motionB);

    pressureEff = clamp01 (p.pressure + p.motion * 0.35f * motionA);
    speedEff    = clamp01 (p.speed + p.motion * 0.35f * motionB);
}

void GestureSource::updateFilters()
{
    const float f0 = (float) p.freq;

    // Per-mode output trim: every gesture leaves this source at a comparable
    // level so switching mode changes character, not loudness.
    static constexpr float kTrim[(int) Mode::Count] = { 1.30f, 5.2f, 0.24f, 0.95f, 0.95f, 28.0f };
    modeTrim = kTrim[(size_t) juce::jlimit (0, (int) Mode::Count - 1, (int) p.mode)];
    tonalBand = p.mode == Mode::Bow || p.mode == Mode::Electrical;

    // ---- bandwidth: resonant band-pass around the note, narrow to wide. The
    //      band is normalised to unit peak gain (not unit noise gain) so a
    //      pitched gesture keeps its level as the band tightens, and the raw
    //      signal is blended back in as the band opens.
    const float q = expMap (1.0f - p.bandwidth, 0.45f, 8.0f);
    band.set (f0, q);
    bandGain = tonalBand ? 1.0f / juce::jmax (0.5f, q) : band.bandpassNoiseGain();
    // Wide keeps the whole spectrum, narrow focuses everything into the band.
    dryMix = std::pow (p.bandwidth, 1.3f) * 0.85f;

    // ---- position: contact point comb tuned to the note. 0.5 is neutral.
    const float period = (float) sr / juce::jmax (20.0f, f0);
    combDelay = juce::jlimit (1, kCombSize - 2, (int) (period * juce::jlimit (0.08f, 0.92f, p.position)));
    combDepth = 2.0f * std::abs (p.position - 0.5f) * 0.75f;

    // ---- per-mode coefficients
    switch (p.mode)
    {
        case Mode::Bow:
            bowInc = juce::jlimit (1.0e-6, 0.24, (double) f0 / sr);
            bowLpCoeff = onePoleCoeff (juce::jlimit (150.0f, (float) sr * 0.40f, f0 * (5.0f + 30.0f * pressureEff)), sr);
            break;

        case Mode::Scrape:
            // Roughness tilts the friction spectrum: smooth surfaces are dull,
            // coarse ones let the high grain through.
            tone.set (juce::jlimit (200.0f, (float) sr * 0.40f, expMap (p.roughness, 700.0f, 9000.0f)), 0.8f);
            break;

        case Mode::Rub:
            rubInc = (float) (expMap (speedEff, 0.4f, 9.0f) / sr);
            tone.set (juce::jlimit (80.0f, (float) sr * 0.35f, 170.0f + 900.0f * pressureEff + 500.0f * rubPhase), 0.9f);
            break;

        case Mode::Breath:
            breathLp.setCutoff (juce::jlimit (200.0f, (float) sr * 0.40f, 420.0f + 3200.0f * pressureEff));
            formant1.set (juce::jlimit (120.0f, (float) sr * 0.40f, 620.0f + 380.0f * pressureEff), 3.0f);
            formant2.set (juce::jlimit (200.0f, (float) sr * 0.40f, 1650.0f + 900.0f * pressureEff), 4.2f);
            break;

        case Mode::Friction:
            // Roughness opens the rasp: dull scuff to a bright, torn scrape.
            tilt.set (900.0f, -0.35f + 1.15f * p.roughness);
            tone.set (juce::jlimit (200.0f, (float) sr * 0.40f, expMap (p.roughness, 2000.0f, 12000.0f)), 0.8f);
            break;

        case Mode::Electrical:
            pulseLp.setCutoff (juce::jlimit (300.0f, (float) sr * 0.42f, 1600.0f + 7000.0f * (0.35f + 0.65f * p.roughness)));
            sputter = 0.18f + 0.82f * pressureEff;
            break;

        default: break;
    }
}

//==============================================================================
inline float GestureSource::renderBow() noexcept
{
    // Stick-slip: the string is carried by the bow for `grip` of the period and
    // snaps back over the rest. The emitted signal is the string's velocity,
    // which is a two level wave with zero mean — band limited with polyBLEP.
    const double inc = bowInc;
    bowPhase += inc;
    if (bowPhase >= 1.0)
    {
        bowPhase -= 1.0;
        const float base = 0.16f + 0.44f * pressureEff;   // bow contact: a short stick is spiky and rich
        bowGrip = juce::jlimit (0.10f, 0.90f, base * (1.0f + p.roughness * 0.4f * shapeRng.nextBipolar()));
        bowSlipNoise = p.roughness * p.roughness * (0.4f + 0.6f * shapeRng.nextFloat());
    }

    const float grip = bowGrip;
    const float slip = grip / juce::jmax (0.05f, 1.0f - grip);
    float v = bowPhase < (double) grip ? 1.0f : -slip;

    // Two discontinuities per period: the slip at `grip` and the catch at 0.
    const double dGrip = bowPhase >= (double) grip ? bowPhase - (double) grip : bowPhase + 1.0 - (double) grip;
    v -= (1.0f + slip) * polyBlep (dGrip, inc);
    v += (1.0f + slip) * polyBlep (bowPhase, inc);

    v *= std::sqrt (juce::jmax (0.02f, (1.0f - grip) / grip));   // constant RMS whatever the grip

    // Friction noise lives in the slip, where the string is sliding.
    if (bowPhase >= (double) grip)
        v += noiseRng.nextBipolar() * bowSlipNoise * 1.6f;

    bowLp += bowLpCoeff * (v - bowLp);
    return bowLp * 0.75f;
}

inline float GestureSource::renderScrape() noexcept
{
    if (--eventCountdown <= 0.0)
    {
        for (auto& g : grains)
        {
            if (g.active) continue;
            const float lengthMs = 0.35f + 2.6f * (1.0f - p.roughness) * shapeRng.nextFloat();
            g.decay = std::exp (-4.0f / juce::jmax (4.0f, lengthMs * 0.001f * (float) sr));
            g.amp = 0.35f + 0.65f * shapeRng.nextFloat();
            g.env = g.amp;
            g.active = true;
            break;
        }
        const double rate = (double) expMap (speedEff, 60.0f, 1500.0f);
        eventCountdown = juce::jmax (2.0, sr / rate * (0.5 + (double) eventRng.nextFloat()));
    }

    grainSum = 0.0f;
    for (auto& g : grains)
    {
        if (! g.active) continue;
        grainSum += g.env;
        g.env *= g.decay;
        if (g.env < 1.0e-4f) g.active = false;
    }

    const float noise = tone.lowpass (noiseRng.nextBipolar()) * tone.lowpassNoiseGain();
    return noise * grainSum * 0.9f;
}

inline float GestureSource::renderRub() noexcept
{
    rubPhase += rubInc;
    if (rubPhase >= 1.0f) rubPhase -= 1.0f;

    const float u = excitation::Tables::hannAt (rubPhase);
    const float amp = 0.22f + 0.95f * u * u;
    const float w = pink.process (noiseRng.nextBipolar());
    const float dark = tone.lowpass (w) * tone.lowpassNoiseGain();
    return dark * amp * 0.85f;
}

inline float GestureSource::renderBreath() noexcept
{
    if (--turbulenceHold <= 0)
    {
        turbulenceTarget = shapeRng.nextFloat();
        const float rate = expMap (speedEff, 1.5f, 26.0f);
        turbulenceHold = juce::jmax (2, (int) (sr / (double) (rate * 2.0f)));
    }
    turbulence += 0.0035f * (turbulenceTarget - turbulence);

    const float w = noiseRng.nextBipolar();
    const float body = breathLp.process (w) * breathLp.impulseGain();
    const float f1 = formant1.bandpass (w) * formant1.bandpassNoiseGain();
    const float f2 = formant2.bandpass (w) * formant2.bandpassNoiseGain();
    const float mix = body * 0.95f + f1 * 0.34f + f2 * 0.15f;
    return mix * (0.40f + 0.95f * turbulence) * 0.55f;
}

inline float GestureSource::renderFriction() noexcept
{
    if (--eventCountdown <= 0.0)
    {
        const float depth = 0.15f + 0.85f * p.roughness;
        grainSum = 1.0f - depth * shapeRng.nextFloat();
        const double rate = (double) expMap (speedEff, 18.0f, 420.0f);
        eventCountdown = juce::jmax (2.0, sr / rate * (0.6 + 0.8 * (double) eventRng.nextFloat()));
    }

    const float w = tone.lowpass (tilt.process (noiseRng.nextBipolar())) * tone.lowpassNoiseGain();
    const float drive = 1.0f + 5.0f * pressureEff;
    const float rasp = fastTanh (w * drive);
    return rasp * grainSum * 0.7f;
}

inline float GestureSource::renderElectrical() noexcept
{
    float impulse = 0.0f;
    if (--pulseCountdown <= 0.0)
    {
        if (eventRng.nextFloat() < sputter)
        {
            const float amp = 0.45f + 0.55f * eventRng.nextFloat();
            impulse = amp * pulseLp.impulsePeakGain() * (eventRng.chance (0.5f) ? 1.0f : -1.0f);
        }
        const double mult = 0.5 + 3.5 * (double) speedEff;
        const double interval = juce::jmax (3.0, sr / juce::jmax (4.0, p.freq * mult));
        const double jitter = 1.0 + (double) p.roughness * 0.7 * (double) eventRng.nextBipolar();
        pulseCountdown = juce::jmax (3.0, interval * juce::jlimit (0.25, 2.0, jitter));
        if (eventRng.nextFloat() < 0.12f * p.roughness) pulseCountdown *= 0.3;   // sputter burst
    }

    const float fizz = noiseRng.nextBipolar() * 0.11f * p.roughness * sputter;
    return pulseDc.process (pulseLp.process (impulse)) * 0.9f + fizz;
}

inline float GestureSource::applyComb (float x) noexcept
{
    comb[(size_t) combWrite] = x;
    const int readIndex = (combWrite - combDelay + kCombSize) & (kCombSize - 1);
    const float delayed = comb[(size_t) readIndex];
    combWrite = (combWrite + 1) & (kCombSize - 1);
    const float out = x - combDepth * delayed;
    return out / std::sqrt (1.0f + combDepth * combDepth);
}

//==============================================================================
void GestureSource::finalise (float* l, float* r, int n, const RenderContext& ctx)
{
    float peak = 0.0f;
    int bad = 0;

    for (int i = 0; i < n; ++i)
    {
        float a = dcL.process (l[i]);
        float b = dcR.process (r[i]);
        if (! std::isfinite (a) || ! std::isfinite (b)) { a = 0.0f; b = 0.0f; ++bad; }
        a = softLimit (a);
        b = softLimit (b);
        l[i] = a;
        r[i] = b;
        peak = juce::jmax (peak, std::abs (a), std::abs (b));
    }

    if (bad > 0)
    {
        dcL.reset(); dcR.reset();
        pink.reset(); tilt.sanitise(); band.reset(); tone.reset();
        formant1.reset(); formant2.reset(); pulseLp.reset(); pulseDc.reset(); breathLp.reset();
        comb.fill (0.0f);
        bowLp = 0.0f;
        for (auto& g : grains) g.active = false;
        if (ctx.diagnostics != nullptr)
            ctx.diagnostics->safety.note (SafetyEvent::NaN, Subsystem::Source, -1, bad);
    }

    lastEnergy = peak;
}

void GestureSource::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    if (n <= 0) return;
    readParams (ctx, note);

    const float velocityGain = 0.35f + 0.65f * p.velocity;

    for (int i = 0; i < n; ++i)
    {
        if (--controlCountdown <= 0)
        {
            updateControl();
            updateFilters();
            controlCountdown = kControlSamples;
        }

        float x = 0.0f;
        switch (p.mode)
        {
            case Mode::Bow:        x = renderBow();        break;
            case Mode::Scrape:     x = renderScrape();     break;
            case Mode::Rub:        x = renderRub();        break;
            case Mode::Breath:     x = renderBreath();     break;
            case Mode::Friction:   x = renderFriction();   break;
            case Mode::Electrical: x = renderElectrical(); break;
            default: break;
        }

        x = applyComb (x * modeTrim);

        const float shaped = band.bandpass (x) * bandGain;
        x = shaped * (1.0f - dryMix) + x * dryMix;

        const float target = gate ? 1.0f : 0.0f;
        env += (gate ? attackCoeff : releaseCoeff) * (target - env);
        if (! gate && env < 1.0e-5f) env = 0.0f;

        // Pressure always raises the level; velocity scales the whole gesture.
        const float out = x * env * p.level * (0.25f + 0.75f * pressureEff) * velocityGain * 0.55f;
        l[i] = out;
        r[i] = out;
    }

    finalise (l, r, n, ctx);
}

} // namespace am
