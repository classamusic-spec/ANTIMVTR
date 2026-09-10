#include "ModSources.h"

namespace am
{

namespace
{
    constexpr double kDivisionBeats[] =
    {
        32.0,      // 8/1
        16.0,      // 4/1
        8.0,       // 2/1
        4.0,       // 1/1
        2.0,       // 1/2
        1.0,       // 1/4
        0.5,       // 1/8
        0.25,      // 1/16
        0.125,     // 1/32
        2.0 / 3.0, // 1/4T
        1.0 / 3.0, // 1/8T
        1.0 / 6.0, // 1/16T
        1.5,       // 1/4D
        0.75,      // 1/8D
        0.375      // 1/16D
    };

    constexpr int kNumDivisions = (int) (sizeof (kDivisionBeats) / sizeof (kDivisionBeats[0]));

    inline float frac01 (double v) noexcept
    {
        if (! std::isfinite (v)) return 0.0f;
        const double f = v - std::floor (v);
        return (float) juce::jlimit (0.0, 0.99999994, f);
    }

    /** Phase skew: the first half of the waveform occupies `sym` of the cycle. */
    inline float skewPhase (float p, float sym) noexcept
    {
        sym = juce::jlimit (0.02f, 0.98f, sym);
        return p < sym ? 0.5f * p / sym
                       : 0.5f + 0.5f * (p - sym) / (1.0f - sym);
    }

    inline float sineAt (float q) noexcept     { return std::sin ((float) kTwoPi * q); }
    inline float triangleAt (float q) noexcept { return q < 0.25f ? 4.0f * q : (q < 0.75f ? 2.0f - 4.0f * q : 4.0f * q - 4.0f); }
    inline float sawAt (float q) noexcept      { return 2.0f * q - 1.0f; }
    inline float squareAt (float q) noexcept   { return q < 0.5f ? 1.0f : -1.0f; }

    inline float smoothStep01 (float t) noexcept
    {
        t = clamp01 (t);
        return t * t * (3.0f - 2.0f * t);
    }

    /** Envelope shape exponent: curve 0 -> 0.25 (snappy), 0.5 -> 1 (linear), 1 -> 4 (soft). */
    inline float curveExponent (float curve) noexcept
    {
        const float c = clamp01 (curve);
        if (std::abs (c - 0.5f) < 1.0e-4f) return 1.0f;
        return std::pow (4.0f, (c - 0.5f) * 2.0f);
    }

    //--------------------------------------------------------------------------
    // Parameter lookup per slot. The slot index is 0-based.

    Param lfoParam (int slot, int which) noexcept
    {
        static constexpr Param table[4][9] =
        {
            { Param::lfo1Rate, Param::lfo1Shape, Param::lfo1Sync, Param::lfo1Division, Param::lfo1Phase, Param::lfo1Symmetry, Param::lfo1Depth, Param::lfo1Retrig, Param::lfo1Fade },
            { Param::lfo2Rate, Param::lfo2Shape, Param::lfo2Sync, Param::lfo2Division, Param::lfo2Phase, Param::lfo2Symmetry, Param::lfo2Depth, Param::lfo2Retrig, Param::lfo2Fade },
            { Param::lfo3Rate, Param::lfo3Shape, Param::lfo3Sync, Param::lfo3Division, Param::lfo3Phase, Param::lfo3Symmetry, Param::lfo3Depth, Param::lfo3Retrig, Param::lfo3Fade },
            { Param::lfo4Rate, Param::lfo4Shape, Param::lfo4Sync, Param::lfo4Division, Param::lfo4Phase, Param::lfo4Symmetry, Param::lfo4Depth, Param::lfo4Retrig, Param::lfo4Fade }
        };
        return table[juce::jlimit (0, 3, slot)][juce::jlimit (0, 8, which)];
    }

    Param envParam (int slot, int which) noexcept
    {
        static constexpr Param table[4][6] =
        {
            { Param::env1Attack, Param::env1Decay, Param::env1Sustain, Param::env1Release, Param::env1Curve, Param::env1Loop },
            { Param::env2Attack, Param::env2Decay, Param::env2Sustain, Param::env2Release, Param::env2Curve, Param::env2Loop },
            { Param::env3Attack, Param::env3Decay, Param::env3Sustain, Param::env3Release, Param::env3Curve, Param::env3Loop },
            { Param::env4Attack, Param::env4Decay, Param::env4Sustain, Param::env4Release, Param::env4Curve, Param::env4Loop }
        };
        return table[juce::jlimit (0, 3, slot)][juce::jlimit (0, 5, which)];
    }

    Param chaosParam (int slot, int which) noexcept
    {
        static constexpr Param table[4][6] =
        {
            { Param::chaos1Type, Param::chaos1Rate, Param::chaos1Depth, Param::chaos1Stability, Param::chaos1Symmetry, Param::chaos1Seed },
            { Param::chaos2Type, Param::chaos2Rate, Param::chaos2Depth, Param::chaos2Stability, Param::chaos2Symmetry, Param::chaos2Seed },
            { Param::chaos3Type, Param::chaos3Rate, Param::chaos3Depth, Param::chaos3Stability, Param::chaos3Symmetry, Param::chaos3Seed },
            { Param::chaos4Type, Param::chaos4Rate, Param::chaos4Depth, Param::chaos4Stability, Param::chaos4Symmetry, Param::chaos4Seed }
        };
        return table[juce::jlimit (0, 3, slot)][juce::jlimit (0, 5, which)];
    }
}

//==============================================================================
double lfoDivisionBeats (int divisionIndex) noexcept
{
    return kDivisionBeats[(size_t) juce::jlimit (0, kNumDivisions - 1, divisionIndex)];
}

float applyModCurve (float v, float curve) noexcept
{
    if (! std::isfinite (v)) return 0.0f;
    if (std::abs (curve) < 1.0e-4f) return v;
    const float magnitude = std::abs (juce::jlimit (-1.0f, 1.0f, v));
    const float exponent = std::pow (4.0f, juce::jlimit (-1.0f, 1.0f, curve));
    const float shaped = std::pow (magnitude, exponent);
    return v < 0.0f ? -shaped : shaped;
}

//==============================================================================
ModLFO::Settings ModLFO::settingsFor (const ParamValues& p, int index) noexcept
{
    Settings s;
    s.rate     = paramValue (p, lfoParam (index, 0));
    s.shape    = (LFOShape) juce::jlimit (0, (int) LFOShape::Count - 1, paramChoice (p, lfoParam (index, 1)));
    s.sync     = paramBool (p, lfoParam (index, 2));
    s.division = paramChoice (p, lfoParam (index, 3));
    s.phase    = paramValue (p, lfoParam (index, 4));
    s.symmetry = paramValue (p, lfoParam (index, 5));
    s.depth    = paramValue (p, lfoParam (index, 6));
    s.retrig   = paramBool (p, lfoParam (index, 7));
    s.fade     = paramValue (p, lfoParam (index, 8));
    return s;
}

double ModLFO::frequencyFor (const Settings& s, const TransportInfo& t) noexcept
{
    if (s.sync)
    {
        const double bpm = juce::jlimit (5.0, 999.0, std::isfinite (t.bpm) ? t.bpm : 120.0);
        return bpm / 60.0 / lfoDivisionBeats (s.division);
    }
    const double rate = std::isfinite (s.rate) ? (double) s.rate : 1.0;
    return juce::jlimit (0.001, 400.0, rate);
}

void ModLFO::prepare (double, uint32_t seed) noexcept
{
    rng.reseed (seed);
    reset();
}

void ModLFO::reset() noexcept
{
    ph = 0.0;
    lastShapedPhase = 0.0;
    current = raw = 0.0f;
    fade = 1.0f;
    fadeElapsed = 0.0;
    started = false;
    randomA = randomB = 0.0f;
}

void ModLFO::retrigger() noexcept
{
    ph = 0.0;
    lastShapedPhase = 0.0;
    fadeElapsed = 0.0;
    started = false;
}

float ModLFO::renderShape (const Settings& s, double phase01) noexcept
{
    const float p = frac01 (phase01);

    if (! started)
    {
        randomA = rng.nextBipolar();
        randomB = rng.nextBipolar();
        started = true;
    }
    else if (p < lastShapedPhase)   // wrapped: new random target
    {
        randomA = randomB;
        randomB = rng.nextBipolar();
    }
    lastShapedPhase = p;

    switch (s.shape)
    {
        case LFOShape::Random:       return randomB;
        case LFOShape::SmoothRandom: return randomA + (randomB - randomA) * smoothStep01 (p);

        case LFOShape::Warp:
        {
            // Continuous morph SINE -> TRIANGLE -> SAW -> SQUARE driven by symmetry.
            const float m = juce::jlimit (0.0f, 2.9999f, clamp01 (s.symmetry) * 3.0f);
            const int i = (int) m;
            const float f = m - (float) i;
            const float a = i == 0 ? sineAt (p) : (i == 1 ? triangleAt (p) : sawAt (p));
            const float b = i == 0 ? triangleAt (p) : (i == 1 ? sawAt (p) : squareAt (p));
            return a + (b - a) * f;
        }

        default: break;
    }

    const float q = skewPhase (p, s.symmetry);
    switch (s.shape)
    {
        case LFOShape::Triangle: return triangleAt (q);
        case LFOShape::Saw:      return sawAt (q);
        case LFOShape::Square:   return squareAt (q);
        case LFOShape::Sine:
        default:                 return sineAt (q);
    }
}

void ModLFO::advance (const Settings& s, int numSamples, double sampleRate,
                      const TransportInfo& transport, bool lockToTransport) noexcept
{
    if (numSamples <= 0 || sampleRate <= 0.0) return;

    const bool locked = s.sync && lockToTransport && transport.isPlaying;
    if (locked)
    {
        const double beats = lfoDivisionBeats (s.division);
        const double ppq = std::isfinite (transport.ppqPosition) ? transport.ppqPosition : 0.0;
        ph = frac01 (ppq / beats);
    }

    // The value of a slice is the value at its start; the phase then moves on.
    raw = renderShape (s, ph + (double) clamp01 (s.phase));

    const float depth = clamp01 (std::isfinite (s.depth) ? s.depth : 0.0f);
    const double seconds = (double) numSamples / sampleRate;
    const double fadeSeconds = std::isfinite (s.fade) ? (double) juce::jlimit (0.0f, 60.0f, s.fade) : 0.0;
    fade = fadeSeconds <= 1.0e-4 ? 1.0f : (float) juce::jlimit (0.0, 1.0, fadeElapsed / fadeSeconds);
    current = raw * depth * fade;
    if (! std::isfinite (current)) current = 0.0f;

    fadeElapsed += seconds;
    if (! locked)
    {
        ph += frequencyFor (s, transport) * seconds;
        ph -= std::floor (ph);
        if (! std::isfinite (ph)) ph = 0.0;
    }
}

//==============================================================================
ModEnvelope::Settings ModEnvelope::settingsFor (const ParamValues& p, int index) noexcept
{
    Settings s;
    s.attack  = paramValue (p, envParam (index, 0));
    s.decay   = paramValue (p, envParam (index, 1));
    s.sustain = clamp01 (paramValue (p, envParam (index, 2)));
    s.release = paramValue (p, envParam (index, 3));
    s.curve   = clamp01 (paramValue (p, envParam (index, 4)));
    s.loop    = paramBool (p, envParam (index, 5));
    return s;
}

void ModEnvelope::reset() noexcept
{
    st = Stage::Idle;
    level = 0.0f;
    stagePos = 0.0f;
    releaseFrom = 0.0f;
}

void ModEnvelope::noteOn() noexcept
{
    st = Stage::Attack;
    stagePos = 0.0f;
    level = 0.0f;
}

void ModEnvelope::noteOff() noexcept
{
    if (st == Stage::Idle) return;
    releaseFrom = level;
    st = Stage::Release;
    stagePos = 0.0f;
}

void ModEnvelope::advance (const Settings& s, int numSamples, double sampleRate) noexcept
{
    if (st == Stage::Idle) { level = 0.0f; return; }
    if (numSamples <= 0 || sampleRate <= 0.0) return;

    double remaining = (double) numSamples / sampleRate;
    int guard = 0;
    while (remaining > 0.0 && ++guard <= 16)
    {
        if (st == Stage::Sustain || st == Stage::Idle) break;

        const double duration = std::max (1.0e-5, (double) (st == Stage::Attack ? s.attack
                                                          : st == Stage::Decay  ? s.decay
                                                                                : s.release));
        const double left = (1.0 - (double) stagePos) * duration;
        if (remaining < left)
        {
            stagePos = (float) juce::jlimit (0.0, 1.0, (double) stagePos + remaining / duration);
            remaining = 0.0;
            break;
        }

        remaining -= left;
        stagePos = 0.0f;
        switch (st)
        {
            case Stage::Attack:  st = Stage::Decay; break;
            case Stage::Decay:   st = s.loop ? Stage::Attack : Stage::Sustain; break;
            case Stage::Release: st = Stage::Idle; break;
            default: break;
        }
    }

    // curve: 0 = snappy (fast attack / fast initial decay), 0.5 = linear, 1 = soft.
    // Only the stage in progress pays for a pow(); sustain and idle pay for nothing.
    const float t = clamp01 (stagePos);
    switch (st)
    {
        case Stage::Attack:  level = std::pow (t, curveExponent (s.curve)); break;
        case Stage::Decay:   level = s.sustain + (1.0f - s.sustain) * std::pow (1.0f - t, 1.0f / curveExponent (s.curve)); break;
        case Stage::Sustain: level = s.sustain; break;
        case Stage::Release: level = releaseFrom * std::pow (1.0f - t, 1.0f / curveExponent (s.curve)); break;
        case Stage::Idle:
        default:             level = 0.0f; break;
    }
    if (! std::isfinite (level)) level = 0.0f;
    level = clamp01 (level);
}

//==============================================================================
ChaosGenerator::Settings ChaosGenerator::settingsFor (const ParamValues& p, int index) noexcept
{
    Settings s;
    s.type      = (ChaosType) juce::jlimit (0, (int) ChaosType::Count - 1, paramChoice (p, chaosParam (index, 0)));
    s.rate      = paramValue (p, chaosParam (index, 1));
    s.depth     = clamp01 (paramValue (p, chaosParam (index, 2)));
    s.stability = clamp01 (paramValue (p, chaosParam (index, 3)));
    s.symmetry  = clamp01 (paramValue (p, chaosParam (index, 4)));
    s.seed      = (uint32_t) juce::jlimit (0, 9999, (int) std::lround (paramValue (p, chaosParam (index, 5))));
    return s;
}

void ChaosGenerator::prepare (uint32_t seed) noexcept
{
    currentSeed = seed;
    rng.reseed (hashSeed (seed, 0x0C4A05u));
    reset();
}

void ChaosGenerator::reset() noexcept
{
    state = current = 0.0f;
    previous = target = 0.0f;
    logistic = 0.35f + 0.3f * rng.nextFloat();
    // The attractor's starting point comes from the seed too, so two seeds trace different orbits.
    lx = 0.1 + 4.0 * (double) rng.nextBipolar();
    ly = 4.0 * (double) rng.nextBipolar();
    lz = 20.0 + 6.0 * (double) rng.nextBipolar();
    phase = 0.0;
}

void ChaosGenerator::step (const Settings& s) noexcept
{
    switch (s.type)
    {
        case ChaosType::Walk:
        {
            const float stepSize = 0.5f * (1.0f - 0.9f * s.stability);
            float v = target + rng.nextBipolar() * stepSize;
            // Reflect at the bounds so the walk stays inside -1 … 1 without sticking.
            while (v > 1.0f || v < -1.0f) v = v > 1.0f ? 2.0f - v : -2.0f - v;
            previous = target;
            target = v;
            break;
        }

        case ChaosType::Logistic:
        {
            const float r = 3.55f + 0.44f * (1.0f - s.stability);
            logistic = juce::jlimit (1.0e-4f, 1.0f - 1.0e-4f, r * logistic * (1.0f - logistic));
            previous = target;
            target = logistic * 2.0f - 1.0f;
            break;
        }

        case ChaosType::Targets:
        {
            previous = target;
            target = rng.nextBipolar();
            break;
        }

        case ChaosType::Brownian:
        case ChaosType::Lorenz:
        default:
            break;   // continuous generators integrate in advance()
    }
}

void ChaosGenerator::advance (const Settings& s, int numSamples, double sampleRate) noexcept
{
    if (numSamples <= 0 || sampleRate <= 0.0) return;
    if (s.seed != currentSeed) prepare (s.seed);

    const double dt = (double) numSamples / sampleRate;
    const double rate = juce::jlimit (0.001, 100.0, std::isfinite (s.rate) ? (double) s.rate : 0.5);

    switch (s.type)
    {
        case ChaosType::Brownian:
        {
            const float sigma = 0.9f * (1.0f - 0.85f * s.stability);
            const double leak = 0.3 + 2.0 * (double) s.stability;
            state += rng.nextGaussian() * sigma * (float) std::sqrt (dt * rate);
            state *= (float) std::exp (-dt * rate * leak);
            while (state > 1.0f || state < -1.0f) state = state > 1.0f ? 2.0f - state : -2.0f - state;
            break;
        }

        case ChaosType::Lorenz:
        {
            constexpr double sigma = 10.0, beta = 8.0 / 3.0;
            const double rho = 24.0 + 14.0 * (1.0 - (double) s.stability);
            const double simTime = dt * rate * 3.0;
            const int subSteps = juce::jlimit (1, 64, (int) std::ceil (simTime / 0.004));
            const double h = simTime / (double) subSteps;
            for (int i = 0; i < subSteps; ++i)
            {
                const double dx = sigma * (ly - lx);
                const double dy = lx * (rho - lz) - ly;
                const double dz = lx * ly - beta * lz;
                lx += dx * h; ly += dy * h; lz += dz * h;
            }
            if (! std::isfinite (lx) || ! std::isfinite (ly) || ! std::isfinite (lz)
                || std::abs (lx) > 1.0e4 || std::abs (lz) > 1.0e4)
            {
                lx = 0.1; ly = 0.0; lz = 20.0;
            }
            state = (float) juce::jlimit (-1.0, 1.0, lx / 20.0);
            break;
        }

        default:
        {
            phase += dt * rate;
            int guard = 0;
            while (phase >= 1.0 && ++guard <= 64) { phase -= 1.0; step (s); }
            if (phase >= 1.0) phase = 0.0;

            if (s.type == ChaosType::Targets)
            {
                const float span = 0.05f + 0.95f * s.stability;
                const float g = smoothStep01 ((float) (phase / (double) span));
                state = previous + (target - previous) * g;
            }
            else
            {
                state = target;
            }
            break;
        }
    }

    if (! std::isfinite (state)) state = 0.0f;
    state = juce::jlimit (-1.0f, 1.0f, state);

    // Symmetry biases the distribution without ever leaving -1 … 1.
    float v = state;
    const float bias = (clamp01 (s.symmetry) - 0.5f) * 2.0f;
    if (std::abs (bias) > 1.0e-4f)
    {
        const float u = clamp01 (v * 0.5f + 0.5f);
        v = std::pow (u, std::pow (4.0f, -bias)) * 2.0f - 1.0f;
    }

    current = juce::jlimit (-1.0f, 1.0f, v) * clamp01 (s.depth);
    if (! std::isfinite (current)) current = 0.0f;
}

//==============================================================================
void NoteSourceValues::update (const NoteState& note, float bendRangeSemis) noexcept
{
    velocity  = clamp01 (note.velocity);
    keyTrack  = juce::jlimit (-1.0f, 1.0f, (float) (note.midiNote - kKeyTrackCentre) / 60.0f);
    pressure  = clamp01 (note.pressure);
    modWheel  = clamp01 (note.modWheel);
    pitchBend = bendRangeSemis > 0.01f ? juce::jlimit (-1.0f, 1.0f, note.pitchBendSemis / bendRangeSemis) : 0.0f;
    timbre    = clamp01 (note.timbre);
    gate      = note.gate ? 1.0f : 0.0f;
}

float NoteSourceValues::of (ModSource s) const noexcept
{
    switch (s)
    {
        case ModSource::Velocity:   return velocity;
        case ModSource::KeyTrack:   return keyTrack;
        case ModSource::Pressure:   return pressure;
        case ModSource::ModWheel:   return modWheel;
        case ModSource::PitchBend:  return pitchBend;
        case ModSource::Timbre:     return timbre;
        case ModSource::NoteRandom: return noteRandom;
        case ModSource::Gate:       return gate;
        default:                    return 0.0f;
    }
}

} // namespace am
