#include "ImpactSource.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

using namespace excitation;

namespace
{
    inline float wrap01 (float x) noexcept { return x - std::floor (x); }

    /** Bell / plate style inharmonic series used by METAL STRIKE. */
    constexpr float kMetalRatios[7] = { 1.0f, 2.32f, 3.91f, 5.62f, 7.44f, 9.35f, 11.34f };

    /** Base peak of one strike at level 1 and velocity 1; the soft limiter holds overlaps. */
    constexpr float kStrikePeak = 0.82f;
}

//==============================================================================
void ImpactSource::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);
    sr = sampleRate;
    warmTables();

    combSize = (int) (sr * 0.08) + 8;          // covers half a period down to ~7 Hz
    comb.assign ((size_t) combSize, 0.0f);

    for (auto& s : strikes)
    {
        s.svf.prepare (sr);
        s.lp.prepare (sr);
        s.tilt.prepare (sr);
    }
    dc.prepare (sr, 8.0f);

    reset();
}

void ImpactSource::reset()
{
    numStrikes = 0;
    gate = false;
    pendingStrike = false;
    repeatCountdown = 0.0;
    strikeCounter = 0;
    combWrite = 0;
    combDelay = 0;
    std::fill (comb.begin(), comb.end(), 0.0f);
    dc.reset();
    lastEnergy = 0.0f;
}

void ImpactSource::noteOn (const NoteState& note, const ParamValues& params)
{
    juce::ignoreUnused (params);
    gate = true;
    noteId = note.noteId;
    strikeCounter = 0;
    repeatRng.reseed (hashSeed (note.noteId, 0x1A9AC7u));
    pendingStrike = true;
    repeatCountdown = 0.0;
}

void ImpactSource::noteOff()
{
    gate = false;
}

bool ImpactSource::isActive() const noexcept
{
    return numStrikes > 0 || pendingStrike || (gate && rateHz > 0.0f);
}

//==============================================================================
void ImpactSource::readParams (const RenderContext& ctx, const NoteState& note)
{
    p.mode       = (Mode) juce::jlimit (0, (int) Mode::Count - 1, ctx.choice (Param::impactMode));
    p.level      = clamp01 (ctx.param (Param::impactLevel));
    p.hardness   = clamp01 (ctx.param (Param::impactHardness));
    p.brightness = clamp01 (ctx.param (Param::impactBrightness));
    p.length     = clamp01 (ctx.param (Param::impactLength));
    p.velSens    = clamp01 (ctx.param (Param::impactVelocity));
    p.curve      = clamp01 (ctx.param (Param::impactCurve));
    p.random     = clamp01 (ctx.param (Param::impactRandom));
    p.rate       = clamp01 (ctx.param (Param::impactRate));
    p.velocity   = clamp01 (note.velocity);
    p.freq       = juce::jlimit ((double) kMinFrequencyHz, sr * 0.45, note.frequency);

    rateHz = p.rate <= 0.002f ? 0.0f : expMap (p.rate, 0.5f, 40.0f);

    // Pluck comb: the excitation point along the "string", 2 % .. 50 % of a period.
    const double period = sr / juce::jmax (1.0, p.freq);
    combDelay = juce::jlimit (1, combSize - 1, (int) (period * (0.02 + 0.48 * (double) p.hardness)));
}

double ImpactSource::nextInterval() noexcept
{
    const double base = sr / (double) juce::jmax (0.05f, rateHz);
    const double jitter = 1.0 + (double) (p.random * repeatRng.nextBipolar() * 0.5f);
    return juce::jmax (8.0, base * jitter);
}

//==============================================================================
void ImpactSource::spawnStrike()
{
    int slot = -1;
    if (numStrikes < kMaxStrikes)
    {
        slot = numStrikes++;
    }
    else
    {
        int best = 0;
        float lowest = 1.0e9f;
        for (int k = 0; k < numStrikes; ++k)
        {
            const float v = strikes[(size_t) k].level();
            if (v < lowest) { lowest = v; best = k; }
        }
        if (lowest > 0.15f) return;     // stealing here would click: drop the strike instead
        slot = best;
    }

    Rng rng (hashSeed (noteId * 2654435761u + strikeCounter, 0x517A1Cu));
    ++strikeCounter;
    initStrike (strikes[(size_t) slot], rng);
}

void ImpactSource::initStrike (Strike& s, Rng& rng)
{
    s = Strike();
    s.svf.prepare (sr);
    s.lp.prepare (sr);
    s.tilt.prepare (sr);
    s.rng.reseed (rng.next());

    s.mode  = p.mode;
    s.curve = p.curve;

    const float rnd       = p.random;
    const float ampJit    = 1.0f + rnd * rng.nextBipolar() * 0.45f;
    const float bright    = clamp01 (p.brightness + rnd * rng.nextBipolar() * 0.3f);
    const float velCurve  = p.velocity * p.velocity * 0.6f + p.velocity * 0.4f;
    const float velAmp    = lerp (1.0f, velCurve, p.velSens);
    const float baseAmp   = kStrikePeak * juce::jlimit (0.05f, 1.6f, ampJit) * velAmp;
    const float nyquist   = (float) (sr * 0.45);

    float tau = 0.03f;      // seconds, main decay
    float gain = 1.0f;

    switch (p.mode)
    {
        case Mode::Impulse:
        {
            const float cut = juce::jlimit (60.0f, juce::jmin (18000.0f, (float) (sr * 0.30)),
                                            expMap (p.hardness, 400.0f, 16000.0f) * expMap (p.length, 2.0f, 0.15f));
            s.lp.setCutoff (cut);
            s.tilt.set (1200.0f, (bright - 0.5f) * 2.0f);
            s.spike = true;
            gain = s.lp.impulsePeakGain();
            tau = juce::jmax (0.0004f, 4.0f / (2.0f * (float) kPi * s.lp.getCutoff()));
            s.attackInc = 1.0f;                 // the low-pass itself limits the slew
            break;
        }

        case Mode::Click:
        {
            tau = expMap (p.length, 0.0002f, 0.02f);
            const float centre = juce::jlimit (60.0f, nyquist, expMap (bright, 400.0f, 12000.0f));
            s.svf.set (centre, 0.6f + p.hardness * 4.0f);
            gain = s.svf.bandpassNoiseGain() * 1.9f;
            s.noiseAmt = 1.0f;
            s.attackInc = 1.0f / juce::jmax (2.0f, (float) (sr * 0.0003 * (1.0 - 0.9 * (double) p.hardness)));
            break;
        }

        case Mode::Pluck:
        {
            tau = expMap (p.length, 0.001f, 0.05f);
            const float cut = juce::jlimit (100.0f, nyquist, (float) p.freq * expMap (bright, 3.0f, 80.0f));
            s.lp.setCutoff (cut);
            gain = s.lp.impulseGain() * 1.8f;
            s.noiseAmt = 1.0f;
            s.attackInc = 1.0f / juce::jmax (2.0f, (float) (sr * 0.0004));
            break;
        }

        case Mode::NoiseStrike:
        {
            tau = expMap (p.length, 0.005f, 1.5f);
            const float cut = juce::jlimit (80.0f, nyquist, expMap (bright, 300.0f, 16000.0f));
            s.svf.set (cut, 0.7f + p.hardness * 2.5f);
            gain = s.svf.lowpassNoiseGain() * 1.7f;
            s.noiseAmt = 1.0f;
            s.attackInc = 1.0f / juce::jmax (2.0f, (float) (sr * 0.0015 * (1.0 - 0.92 * (double) p.hardness)));
            break;
        }

        case Mode::MetalStrike:
        {
            tau = expMap (p.length, 0.02f, 1.5f);
            const float root = juce::jlimit (20.0f, nyquist * 0.5f, (float) p.freq * expMap (bright, 1.0f, 5.0f));
            const float slope = 1.2f - bright * 0.9f;
            float sumSq = 0.0f;
            for (int k = 0; k < kMetalPartials; ++k)
            {
                const float ratio = kMetalRatios[k] * (1.0f + p.hardness * 0.35f * rng.nextBipolar());
                const float f = root * ratio;
                s.mInc[k]   = f < nyquist ? f / (float) sr : 0.0f;
                s.mPhase[k] = rng.nextFloat();
                s.mEnv[k]   = 1.0f;
                s.mAmp[k]   = f < nyquist ? std::pow (1.0f / (1.0f + (float) k), slope) * (0.6f + 0.4f * rng.nextFloat())
                                          : 0.0f;
                const float pTau = juce::jmax (0.002f, tau / (1.0f + (float) k * 0.55f));
                s.mDec[k]   = std::exp (-1.0f / (float) juce::jmax (2.0, (double) pTau * sr));
                sumSq += s.mAmp[k] * s.mAmp[k];
            }
            const float norm = 1.0f / std::sqrt (juce::jmax (1.0e-6f, sumSq));
            for (auto& a : s.mAmp) a *= norm;
            s.clickAmt = 0.35f * p.hardness;
            s.lp.setCutoff (juce::jlimit (200.0f, nyquist, expMap (bright, 2000.0f, 14000.0f)));
            s.decayClick = std::exp (-1.0f / (float) juce::jmax (2.0, 0.0015 * sr));
            gain = 1.5f;
            s.attackInc = 1.0f / juce::jmax (2.0f, (float) (sr * 0.0002));
            break;
        }

        case Mode::DampedSine:
        {
            tau = expMap (p.length, 0.005f, 2.0f);
            s.inc = (float) (juce::jlimit (10.0, (double) nyquist, p.freq) / sr);
            s.harm2 = bright * 0.45f;
            s.clickAmt = 0.5f * p.hardness;
            s.lp.setCutoff (juce::jlimit (200.0f, nyquist, expMap (bright, 1500.0f, 12000.0f)));
            s.decayClick = std::exp (-1.0f / (float) juce::jmax (2.0, 0.0015 * sr));
            gain = 1.0f;
            s.attackInc = 1.0f / juce::jmax (2.0f, (float) (sr * 0.0002));
            break;
        }

        case Mode::MembraneHit:
        default:
        {
            tau = expMap (p.length, 0.02f, 1.2f);
            const double target = juce::jlimit (12.0, (double) nyquist, p.freq * 0.5);
            s.incTarget = (float) (target / sr);
            s.inc = (float) juce::jmin ((double) nyquist / sr, (double) s.incTarget * (1.0 + 3.0 * (double) p.hardness));
            s.glide = 1.0f - std::exp (-1.0f / (float) juce::jmax (2.0, 0.025 * sr));
            s.clickAmt = 0.55f * bright;
            s.lp.setCutoff (juce::jlimit (200.0f, nyquist, expMap (bright, 500.0f, 8000.0f)));
            s.decayClick = std::exp (-1.0f / (float) juce::jmax (2.0, 0.004 * sr));
            gain = 1.0f;
            s.attackInc = 1.0f / juce::jmax (2.0f, (float) (sr * 0.0003));
            break;
        }
    }

    s.decay     = std::exp (-1.0f / (float) juce::jmax (2.0, (double) tau * sr));
    s.decayFast = std::exp (-1.0f / (float) juce::jmax (2.0, (double) tau * 0.15 * sr));
    s.env = 1.0f;
    s.envFast = 1.0f;
    s.envClick = 1.0f;
    s.attack = 0.0f;
    s.age = 0;
    s.amp = baseAmp * gain;
}

//==============================================================================
inline float ImpactSource::renderStrike (Strike& s) noexcept
{
    s.env *= s.decay;
    s.envFast *= s.decayFast;

    float e;
    if (s.curve < 0.5f)
    {
        e = s.env * lerp (s.envFast, 1.0f, s.curve * 2.0f);
    }
    else
    {
        const float rounded = s.env * (2.0f - s.env);
        e = lerp (s.env, rounded, (s.curve - 0.5f) * 2.0f);
    }

    if (s.attack < 1.0f) s.attack = juce::jmin (1.0f, s.attack + s.attackInc);
    e *= s.attack;

    float x = 0.0f;
    switch (s.mode)
    {
        case Mode::Impulse:
        {
            const float in = s.spike ? 1.0f : 0.0f;
            s.spike = false;
            x = s.tilt.process (s.lp.process (in));
            break;
        }

        case Mode::Click:
            x = s.svf.bandpass (s.rng.nextBipolar()) * s.noiseAmt;
            break;

        case Mode::Pluck:
            x = s.lp.process (s.rng.nextBipolar()) * s.noiseAmt;
            break;

        case Mode::NoiseStrike:
            x = s.svf.lowpass (s.rng.nextBipolar()) * s.noiseAmt;
            break;

        case Mode::MetalStrike:
        {
            for (int k = 0; k < kMetalPartials; ++k)
            {
                s.mEnv[k] *= s.mDec[k];
                x += excitation::Tables::sineAt (s.mPhase[k]) * s.mAmp[k] * s.mEnv[k];
                s.mPhase[k] += s.mInc[k];
                if (s.mPhase[k] >= 1.0f) s.mPhase[k] -= 1.0f;
            }
            s.envClick *= s.decayClick;
            x += s.lp.process (s.rng.nextBipolar()) * s.clickAmt * s.envClick;
            break;
        }

        case Mode::DampedSine:
        {
            const float a = excitation::Tables::sineAt (s.phase);
            const float b = excitation::Tables::sineAt (wrap01 (s.phase * 2.0f));
            x = a * (1.0f - s.harm2) + b * s.harm2;
            s.phase += s.inc;
            if (s.phase >= 1.0f) s.phase -= 1.0f;
            s.envClick *= s.decayClick;
            x += s.lp.process (s.rng.nextBipolar()) * s.clickAmt * s.envClick;
            break;
        }

        case Mode::MembraneHit:
        default:
        {
            s.inc += (s.incTarget - s.inc) * s.glide;
            x = excitation::Tables::sineAt (s.phase);
            s.phase += s.inc;
            if (s.phase >= 1.0f) s.phase -= 1.0f;
            s.envClick *= s.decayClick;
            x += s.lp.process (s.rng.nextBipolar()) * s.clickAmt * s.envClick;
            break;
        }
    }

    ++s.age;
    return x * e * s.amp;
}

//==============================================================================
void ImpactSource::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    readParams (ctx, note);

    const bool  pluckComb = p.mode == Mode::Pluck && combDelay >= 1;
    const float g = p.level;
    float peak = 0.0f;
    int bad = 0;

    for (int i = 0; i < n; ++i)
    {
        if (pendingStrike)
        {
            pendingStrike = false;
            spawnStrike();
            repeatCountdown = rateHz > 0.0f ? nextInterval() : 1.0e12;
        }
        else if (gate && rateHz > 0.0f)
        {
            repeatCountdown -= 1.0;
            while (repeatCountdown <= 0.0)
            {
                spawnStrike();
                repeatCountdown += nextInterval();
            }
        }

        float x = 0.0f;
        for (int k = 0; k < numStrikes; )
        {
            Strike& s = strikes[(size_t) k];
            x += renderStrike (s);
            if (s.age > 8 && s.level() < 1.0e-5f) strikes[(size_t) k] = strikes[(size_t) --numStrikes];
            else ++k;
        }

        if (pluckComb)
        {
            int rd = combWrite - combDelay;
            if (rd < 0) rd += combSize;
            const float d = comb[(size_t) rd];
            comb[(size_t) combWrite] = x;
            if (++combWrite >= combSize) combWrite = 0;
            x = (x - 0.85f * d) * 0.70f;
        }

        float v = dc.process (x * g);
        if (! std::isfinite (v)) { v = 0.0f; ++bad; }
        v = softLimit (v);
        l[i] = v;
        peak = juce::jmax (peak, std::abs (v));
    }

    juce::FloatVectorOperations::copy (r, l, n);

    if (bad > 0)
    {
        numStrikes = 0;
        dc.reset();
        std::fill (comb.begin(), comb.end(), 0.0f);
        if (ctx.diagnostics != nullptr)
            ctx.diagnostics->safety.note (SafetyEvent::NaN, Subsystem::Source, -1, bad);
    }

    lastEnergy = peak;
}

} // namespace am
