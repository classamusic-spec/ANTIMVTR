#include "DustSource.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

using namespace excitation;

namespace
{
    /** Exponential deviate with mean 1, bounded so a scheduler can never stall. */
    inline float expRandom (Rng& rng) noexcept
    {
        const float u = juce::jlimit (2.0e-3f, 0.999f, rng.nextFloat());
        return juce::jmin (6.0f, -std::log (u));
    }

    inline float wrap01 (float x) noexcept { return x - std::floor (x); }

    /** Per-mode output gains: white/pink/brown/blue are matched to ~0.25 RMS at level 1. */
    constexpr float kWhiteGain = 0.43f;
    constexpr float kPinkGain  = 0.44f;
    constexpr float kBrownGain = 0.44f;
    constexpr float kBlueGain  = 0.52f;
}

//==============================================================================
void DustSource::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);
    sr = sampleRate;
    warmTables();

    tiltL.prepare (sr);      tiltR.prepare (sr);
    bandL.prepare (sr);      bandR.prepare (sr);
    crackleLpL.prepare (sr); crackleLpR.prepare (sr);
    dcL.prepare (sr, 10.0f); dcR.prepare (sr, 10.0f);
    gate.maxRamp = (float) (sr * 0.002);

    reset();
}

void DustSource::reset()
{
    pinkL.reset();  pinkR.reset();
    brownL.reset(); brownR.reset();
    diffL.reset();  diffR.reset();
    tiltL.reset();  tiltR.reset();
    bandL.reset();  bandR.reset();
    crackleLpL.reset(); crackleLpR.reset();
    dcL.reset();    dcR.reset();
    gate.reset();
    clearEvents();
    wanderValue = wanderTarget = wanderPhase = 0.0f;
    controlCountdown = 0;
    lastEnergy = 0.0f;
    lastSeedParam = -1;
}

void DustSource::clearEvents()
{
    numCrackle = numPulses = numGrains = 0;
    eventCountdown = 0.0;
    pulseCountdown = 0.0;
    pulseSlot = 0;
}

void DustSource::reseed (uint32_t seedValue, uint32_t noteId)
{
    baseSeed = hashSeed (seedValue, 0x0D05700Du ^ (noteId * 2654435761u));
    noiseCommon.reseed (hashSeed (baseSeed, 0x11u));
    noiseLeft  .reseed (hashSeed (baseSeed, 0x22u));
    noiseRight .reseed (hashSeed (baseSeed, 0x33u));
    gateRng    .reseed (hashSeed (baseSeed, 0x44u));
    eventRng   .reseed (hashSeed (baseSeed, 0x55u));
    grainRng   .reseed (hashSeed (baseSeed, 0x66u));
    shapeRng   .reseed (hashSeed (baseSeed, 0x77u));
    currentNoteId = noteId;
    initPartials();
}

void DustSource::initPartials()
{
    Rng rng (hashSeed (baseSeed, 0xF702E4u));
    for (auto& q : partials)
    {
        q.octave     = rng.nextBipolar();
        q.ampRand    = rng.nextFloat();
        q.pan        = rng.nextBipolar();
        q.rRand      = rng.nextFloat();
        q.driftRate  = 0.03f + 0.32f * rng.nextFloat();
        q.driftPhase = rng.nextFloat();

        const float phase0 = rng.nextFloat();
        q.c = Tables::sineAt (wrap01 (phase0 + 0.25f));
        q.s = Tables::sineAt (phase0);
        q.dCos = 1.0f;
        q.dSin = 0.0f;
        q.gainL = q.gainRc = q.gainRs = 0.0f;
    }
    controlCountdown = 0;
}

void DustSource::noteOn (const NoteState& note, const ParamValues& params)
{
    lastSeedParam = (int) std::lround (paramValue (params, Param::dustSeed));
    reseed ((uint32_t) juce::jmax (0, lastSeedParam), note.noteId);
    clearEvents();
    gate.reset();
    activeMode = (Mode) juce::jlimit (0, (int) Mode::Count - 1, paramChoice (params, Param::dustMode));
}

void DustSource::noteOff() {}

//==============================================================================
void DustSource::readParams (const RenderContext& ctx, const NoteState& note)
{
    p.mode       = (Mode) juce::jlimit (0, (int) Mode::Count - 1, ctx.choice (Param::dustMode));
    p.level      = clamp01 (ctx.param (Param::dustLevel));
    p.density    = clamp01 (ctx.param (Param::dustDensity));
    p.color      = clamp01 (ctx.param (Param::dustColor));
    p.grain      = clamp01 (ctx.param (Param::dustGrain));
    p.jitter     = clamp01 (ctx.param (Param::dustJitter));
    p.pitchSemis = juce::jlimit (-24.0f, 24.0f, ctx.param (Param::dustPitch));
    p.position   = clamp01 (ctx.param (Param::dustPosition));
    p.spread     = clamp01 (ctx.param (Param::dustSpread));
    p.stereo     = clamp01 (ctx.param (Param::dustStereo));
    p.seed       = (int) std::lround (ctx.param (Param::dustSeed));

    p.freq = juce::jlimit ((double) kMinFrequencyHz, sr * 0.45,
                           note.frequency * std::pow (2.0, (double) p.pitchSemis / 12.0));

    if (p.seed != lastSeedParam)
    {
        lastSeedParam = p.seed;
        reseed ((uint32_t) juce::jmax (0, p.seed), currentNoteId);
        clearEvents();
    }

    if (p.mode != activeMode)
    {
        activeMode = p.mode;
        clearEvents();
        gate.reset();
    }

    const float a = (float) kPi * 0.5f * p.stereo;
    corrCommon = std::cos (a);
    corrIndep  = std::sin (a);
    gateSegment = (float) sr * expMap (p.grain, 0.002f, 0.25f);
}

inline void DustSource::nextNoise (float& wl, float& wr) noexcept
{
    const float c = noiseCommon.nextBipolar();
    const float a = noiseLeft.nextBipolar();
    const float b = noiseRight.nextBipolar();
    wl = corrCommon * c + corrIndep * a;
    wr = corrCommon * c + corrIndep * b;
}

//==============================================================================
void DustSource::renderColoured (float* l, float* r, int n)
{
    const float tilt = (p.color - 0.5f) * 2.0f;
    tiltL.set (900.0f, tilt);
    tiltR.set (900.0f, tilt);

    float g = kWhiteGain;
    switch (activeMode)
    {
        case Mode::Pink:  g = kPinkGain;  break;
        case Mode::Brown: g = kBrownGain; break;
        case Mode::Blue:  g = kBlueGain;  break;
        default: break;
    }

    for (int i = 0; i < n; ++i)
    {
        float wl, wr;
        nextNoise (wl, wr);

        switch (activeMode)
        {
            case Mode::Pink:
                wl = pinkL.process (wl);  wr = pinkR.process (wr);  break;
            case Mode::Brown:
                wl = brownL.process (wl); wr = brownR.process (wr); break;
            case Mode::Blue:
                wl = diffL.process (pinkL.process (wl));
                wr = diffR.process (pinkR.process (wr));            break;
            default: break;   // WHITE
        }

        wl = tiltL.process (wl);
        wr = tiltR.process (wr);

        const float occ = gate.next (gateRng, p.density, gateSegment, p.jitter);
        l[i] = wl * g * occ;
        r[i] = wr * g * occ;
    }
}

void DustSource::updateFilteredBand()
{
    // Slow random wander of the band centre: rate from `grain`, depth from `jitter`.
    const float rate = expMap (p.grain, 0.15f, 25.0f);
    wanderPhase += (float) ((double) kControlSamples * (double) rate / sr);
    while (wanderPhase >= 1.0f)
    {
        wanderPhase -= 1.0f;
        wanderTarget = shapeRng.nextBipolar();
    }
    const float smooth = 1.0f - std::exp (- (float) kControlSamples / (float) (sr * 0.05));
    wanderValue += (wanderTarget - wanderValue) * smooth;

    const float harmonic = 1.0f + p.position * 7.0f;
    const float centre = juce::jlimit (20.0f, (float) (sr * 0.45),
                                       (float) p.freq * harmonic * fastPow2 (wanderValue * p.jitter * 1.5f));
    const float q = expMap (p.color, 0.7f, 40.0f);
    bandL.set (centre, q);
    bandR.set (centre, q);
}

void DustSource::renderFiltered (float* l, float* r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        if (--controlCountdown <= 0)
        {
            updateFilteredBand();
            controlCountdown = kControlSamples;
        }
        const float g = bandL.bandpassNoiseGain() * 0.21f;

        float wl, wr;
        nextNoise (wl, wr);
        wl = bandL.bandpass (wl) * g;
        wr = bandR.bandpass (wr) * g;
        const float occ = gate.next (gateRng, p.density, gateSegment, p.jitter);
        l[i] = wl * occ;
        r[i] = wr * occ;
    }
}

void DustSource::renderCrackle (float* l, float* r, int n)
{
    const float rate    = expMap (p.density, 1.0f, 2000.0f);          // events / second
    const float burst   = expMap (p.grain, 0.0003f, 0.08f);           // seconds
    const float tauSec  = juce::jmax (2.0e-5f, burst * 0.35f);
    const float decay   = std::exp (-1.0f / (float) juce::jmax (2.0, (double) tauSec * sr));
    const float attack  = 1.0f - std::exp (-1.0f / (float) juce::jmax (2.0, (double) tauSec * sr * 0.12));
    const double meanInterval = juce::jmax (2.0, sr / (double) rate);

    crackleLpL.setCutoff (expMap (p.color, 300.0f, 16000.0f));
    crackleLpR.setCutoff (crackleLpL.getCutoff());
    const float lpGain = crackleLpL.impulseGain();

    // RMS-ish normalisation, but capped so isolated crackles keep their crest factor.
    const float occ = std::sqrt (juce::jmax (1.0e-5f, rate * tauSec * 0.5f));
    const float norm = juce::jlimit (0.08f, 1.25f, 0.30f / occ);

    for (int i = 0; i < n; ++i)
    {
        eventCountdown -= 1.0;
        while (eventCountdown <= 0.0)
        {
            if (numCrackle < kMaxCrackleEvents)
            {
                auto& e = crackle[(size_t) numCrackle++];
                e.active = true;
                e.env = 1.0f - 0.8f * p.jitter * eventRng.nextFloat();
                e.attack = 0.0f;
                e.decayCoeff = decay;
                e.attackCoeff = attack;
                panGains (p.spread * eventRng.nextBipolar(), e.gainL, e.gainR);
            }
            const double interval = meanInterval * (double) lerp (1.0f, expRandom (eventRng), p.jitter);
            eventCountdown += juce::jmax (2.0, interval);
        }

        float envL = 0.0f, envR = 0.0f;
        for (int k = 0; k < numCrackle; )
        {
            auto& e = crackle[(size_t) k];
            e.attack += (1.0f - e.attack) * e.attackCoeff;
            e.env *= e.decayCoeff;
            const float v = e.env * e.attack;
            envL += v * e.gainL;
            envR += v * e.gainR;
            if (e.env < 1.0e-4f) crackle[(size_t) k] = crackle[(size_t) --numCrackle];
            else ++k;
        }

        float wl, wr;
        nextNoise (wl, wr);
        l[i] = envL * crackleLpL.process (wl) * lpGain * norm;
        r[i] = envR * crackleLpR.process (wr) * lpGain * norm;
    }
}

void DustSource::renderImpulse (float* l, float* r, int n)
{
    const double periodSamples = juce::jmax (4.0, sr / juce::jmax (1.0, p.freq));
    const float  perCycle = 1.0f + p.density * 3.0f;                 // 1 .. 4 impulses per cycle
    const int    slots = juce::jlimit (1, 4, (int) std::ceil (perCycle - 1.0e-4f));
    const double slotPeriod = periodSamples / (double) slots;
    const int    len = juce::jlimit (2, (int) (sr * 0.05), (int) (expMap (p.grain, 0.00005f, 0.006f) * (float) sr));
    const float  invLen = 1.0f / (float) len;

    const float tilt = (p.color - 0.5f) * 2.0f;
    tiltL.set (1200.0f, tilt);
    tiltR.set (1200.0f, tilt);

    const float effRate = (float) ((double) perCycle * p.freq);
    const float norm = juce::jlimit (0.10f, 1.0f,
                                     0.34f / std::sqrt (juce::jmax (1.0e-4f, effRate * (float) len / (float) sr * 0.19f)));

    for (int i = 0; i < n; ++i)
    {
        pulseCountdown -= 1.0;
        while (pulseCountdown <= 0.0)
        {
            const int slot = pulseSlot % slots;
            const float prob = slot == 0 ? 1.0f : clamp01 (perCycle - (float) slot);
            if (eventRng.nextFloat() < prob && numPulses < kMaxPulses)
            {
                auto& e = pulses[(size_t) numPulses++];
                e.active = true;
                const int delay = (int) (p.jitter * 0.9f * eventRng.nextFloat() * (float) slotPeriod);
                const int rDelay = (int) (p.stereo * 0.35f * eventRng.nextFloat() * (float) slotPeriod);
                e.ageL = -delay;
                e.ageR = -delay - rDelay;
                e.len = len;
                e.invLen = invLen;
                const float amp = (slot == 0 ? 1.0f : 0.7f) * (1.0f - 0.5f * p.jitter * eventRng.nextFloat());
                panGains (p.spread * eventRng.nextBipolar(), e.gainL, e.gainR);
                e.gainL *= amp;
                e.gainR *= amp;
            }
            ++pulseSlot;
            pulseCountdown += juce::jmax (2.0, slotPeriod);
        }

        float sumL = 0.0f, sumR = 0.0f;
        for (int k = 0; k < numPulses; )
        {
            auto& e = pulses[(size_t) k];
            if (e.ageL >= 0 && e.ageL < e.len) sumL += Tables::pulseAt ((float) e.ageL * e.invLen) * e.gainL;
            if (e.ageR >= 0 && e.ageR < e.len) sumR += Tables::pulseAt ((float) e.ageR * e.invLen) * e.gainR;
            ++e.ageL;
            ++e.ageR;
            if (e.ageL >= e.len && e.ageR >= e.len) pulses[(size_t) k] = pulses[(size_t) --numPulses];
            else ++k;
        }

        l[i] = tiltL.process (sumL) * norm;
        r[i] = tiltR.process (sumR) * norm;
    }
}

void DustSource::renderCloud (float* l, float* r, int n)
{
    const float rate     = expMap (p.density, 1.0f, 160.0f);          // grains / second
    const float grainSec = 0.005f + p.grain * 0.195f;                 // 5 .. 200 ms
    const double meanInterval = juce::jmax (4.0, sr / (double) rate);
    const float overlap  = juce::jmax (1.0f, rate * grainSec);

    // Tonal grains sum coherently when jitter is low; noise grains always sum in power.
    const float incoherent = juce::jmin (1.0f, p.jitter * 3.0f);
    const float toneNorm  = 0.60f * std::pow (overlap, -(1.0f - 0.5f * incoherent));
    const float noiseNorm = 0.60f * std::pow (overlap, -0.5f);

    const float toneAmt  = std::cos (p.color * (float) kPi * 0.5f);
    const float noiseAmt = std::sin (p.color * (float) kPi * 0.5f) * 1.22f;
    const bool  wantNoise = noiseAmt > 1.0e-3f;
    const bool  wideNoise = p.stereo > 0.02f;
    const float rPhaseOffset = p.stereo * 0.25f;

    for (int i = 0; i < n; ++i)
    {
        eventCountdown -= 1.0;
        while (eventCountdown <= 0.0)
        {
            if (numGrains < kMaxGrains)
            {
                auto& g = grains[(size_t) numGrains++];
                g.active = true;
                g.age = 0;
                const float lenJit = 1.0f + p.jitter * grainRng.nextBipolar() * 0.4f;
                g.len = juce::jlimit (8, (int) (sr * 0.5), (int) (grainSec * lenJit * (float) sr));
                g.invLen = 1.0f / (float) g.len;
                const float semis = p.jitter * grainRng.nextBipolar() * 6.0f;
                const double f = juce::jlimit (5.0, sr * 0.45, p.freq * (double) fastPow2 (semis / 12.0f));
                g.inc = (float) (f / sr);
                g.phase = wrap01 (p.position + p.jitter * grainRng.nextFloat());
                g.rPhase = rPhaseOffset;
                g.tone = toneAmt * toneNorm;
                g.noise = noiseAmt * noiseNorm;
                const float amp = 1.0f - 0.5f * p.jitter * grainRng.nextFloat();
                panGains (p.spread * grainRng.nextBipolar(), g.gainL, g.gainR);
                g.gainL *= amp;
                g.gainR *= amp;
                g.rng.reseed (grainRng.next());
            }
            const double interval = meanInterval * (double) lerp (1.0f, expRandom (grainRng), p.jitter);
            eventCountdown += juce::jmax (4.0, interval);
        }

        float sumL = 0.0f, sumR = 0.0f;
        for (int k = 0; k < numGrains; )
        {
            auto& g = grains[(size_t) k];
            const float w = Tables::hannAt ((float) g.age * g.invLen);
            const float s = Tables::sineAt (g.phase);
            const float sR = wideNoise ? Tables::sineAt (wrap01 (g.phase + g.rPhase)) : s;

            float nL = 0.0f, nR = 0.0f;
            if (wantNoise)
            {
                nL = g.rng.nextBipolar();
                nR = wideNoise ? g.rng.nextBipolar() : nL;
            }

            sumL += w * (g.tone * s  + g.noise * nL) * g.gainL;
            sumR += w * (g.tone * sR + g.noise * nR) * g.gainR;

            g.phase += g.inc;
            if (g.phase >= 1.0f) g.phase -= 1.0f;
            if (++g.age >= g.len) grains[(size_t) k] = grains[(size_t) --numGrains];
            else ++k;
        }

        l[i] = sumL;
        r[i] = sumR;
    }
}

void DustSource::updateFrozen()
{
    numPartials = juce::jlimit (24, kMaxPartials, 24 + (int) std::lround (p.density * 40.0f));

    const float centre = juce::jlimit (10.0f, (float) (sr * 0.4), (float) p.freq * fastPow2 (p.position * 2.0f));
    const float spreadOct = 0.25f + p.grain * 3.0f;
    const float tiltExp = 1.0f - p.color * 2.0f;
    const float driftDepth = p.jitter * 0.02f;
    const float nyquist = (float) (sr * 0.45);

    float amps[kMaxPartials];
    float sumSq = 0.0f;
    for (int k = 0; k < numPartials; ++k)
    {
        const float oct = partials[(size_t) k].octave * spreadOct;
        const float a = (0.35f + 0.65f * partials[(size_t) k].ampRand) * fastPow2 (-tiltExp * oct);
        amps[k] = a;
        sumSq += a * a;
    }
    const float norm = 0.26f / std::sqrt (juce::jmax (1.0e-6f, 0.5f * sumSq));

    for (int k = 0; k < numPartials; ++k)
    {
        auto& q = partials[(size_t) k];
        q.driftPhase = wrap01 (q.driftPhase + (float) ((double) kControlSamples * (double) q.driftRate / sr));

        const float oct = q.octave * spreadOct;
        const float f = centre * fastPow2 (oct) * (1.0f + driftDepth * fastSin01 (q.driftPhase));
        const float inc = juce::jlimit (0.0f, nyquist, f) / (float) sr;
        const float w = (float) kTwoPi * inc;
        q.dCos = std::cos (w);
        q.dSin = std::sin (w);

        // Keep the rotator on the unit circle (first order inverse square root).
        const float len = q.c * q.c + q.s * q.s;
        const float fix = 1.5f - 0.5f * juce::jlimit (0.0f, 2.9f, len);
        q.c *= fix;
        q.s *= fix;

        const float amp = amps[k] * norm;
        float gl, gr;
        panGains (q.pan * p.spread, gl, gr);
        const float delta = q.rRand * p.stereo;
        q.gainL  = amp * gl;
        q.gainRc = amp * gr * Tables::sineAt (wrap01 (delta + 0.25f));
        q.gainRs = amp * gr * Tables::sineAt (wrap01 (delta));
    }
}

void DustSource::renderFrozen (float* l, float* r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        if (--controlCountdown <= 0)
        {
            updateFrozen();
            controlCountdown = kControlSamples;
        }

        float sumL = 0.0f, sumR = 0.0f;
        for (int k = 0; k < numPartials; ++k)
        {
            auto& q = partials[(size_t) k];
            const float ns = q.s * q.dCos + q.c * q.dSin;
            const float nc = q.c * q.dCos - q.s * q.dSin;
            q.s = ns;
            q.c = nc;
            sumL += ns * q.gainL;
            sumR += ns * q.gainRc + nc * q.gainRs;
        }
        l[i] = sumL;
        r[i] = sumR;
    }
}

//==============================================================================
void DustSource::finalise (float* l, float* r, int n, const RenderContext& ctx)
{
    const float g = p.level;
    float peak = 0.0f;
    int bad = 0;

    for (int i = 0; i < n; ++i)
    {
        float a = dcL.process (l[i] * g);
        float b = dcR.process (r[i] * g);
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
        pinkL.reset(); pinkR.reset(); brownL.reset(); brownR.reset();
        diffL.reset(); diffR.reset(); tiltL.sanitise(); tiltR.sanitise();
        bandL.reset(); bandR.reset(); crackleLpL.reset(); crackleLpR.reset();
        clearEvents();
        if (ctx.diagnostics != nullptr)
            ctx.diagnostics->safety.note (SafetyEvent::NaN, Subsystem::Source, -1, bad);
    }

    lastEnergy = peak;
}

void DustSource::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    readParams (ctx, note);

    switch (activeMode)
    {
        case Mode::Filtered: renderFiltered (l, r, n); break;
        case Mode::Crackle:  renderCrackle  (l, r, n); break;
        case Mode::Impulse:  renderImpulse  (l, r, n); break;
        case Mode::Cloud:    renderCloud    (l, r, n); break;
        case Mode::Frozen:   renderFrozen   (l, r, n); break;
        default:             renderColoured (l, r, n); break;
    }

    finalise (l, r, n, ctx);
}

} // namespace am
