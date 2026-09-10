#include "FractureEngine.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

namespace
{
    constexpr float kMaxDelaySeconds = 1.0f;   ///< longest per-fragment spectral delay
    constexpr float kMaxFeedback     = 0.96f;  ///< hard ceiling on any feedback coefficient
    constexpr float kRingCeiling     = 2.5f;   ///< delay-line level cap (1.0 = full-scale sine)
    constexpr float kHalfPi          = 1.57079632679489662f;
    constexpr float kTwoPif          = 6.28318530717958648f;

    inline float wrapPi (float x) noexcept
    {
        const float k = std::round (x * (1.0f / kTwoPif));
        return x - k * kTwoPif;
    }

    inline float melOf (float hz) noexcept    { return 2595.0f * std::log10 (1.0f + hz / 700.0f); }
    inline float melToHz (float mel) noexcept { return 700.0f * (std::pow (10.0f, mel / 2595.0f) - 1.0f); }

    /** Snaps a scattered transposition to a consonant interval.

        Arbitrary fractional transpositions beat against the un-shifted fragments
        and turn the effect into phasey mush; octaves, fifths and fourths stack
        into something that still sounds like the instrument. */
    inline float quantiseInterval (float semitones) noexcept
    {
        static constexpr float kIntervals[] = { -24.0f, -19.0f, -12.0f, -7.0f, -5.0f, 0.0f,
                                                  5.0f,   7.0f,  12.0f, 19.0f,  24.0f };
        float best = 0.0f, bestDistance = 1.0e9f;
        for (float v : kIntervals)
        {
            const float d = std::abs (v - semitones);
            if (d < bestDistance) { bestDistance = d; best = v; }
        }
        return best;
    }
}

//==============================================================================
FractureEngine::FractureEngine() = default;
FractureEngine::~FractureEngine() = default;

//==============================================================================
void FractureEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;

    // 1024 at 44.1 / 48 kHz, 2048 at 88.2 / 96 kHz — ~21 ms of latency either way.
    stft.prepare (sr > 64000.0 ? 2048 : 1024);
    latency = stft.latency();

    maxBins   = stft.numBins();
    binStride = 2 * maxBins;
    maxFrames = (int) std::ceil (sr * (double) kMaxDelaySeconds / (double) stft.hop()) + 4;

    radiansPerBin = (float) (kTwoPi * (double) stft.hop() / (double) stft.size());
    binsPerRadian = 1.0f / radiansPerBin;

    for (int ch = 0; ch < 2; ++ch)
    {
        ring[ch].assign ((size_t) maxFrames * (size_t) binStride, 0.0f);
        wetSpec[ch].assign ((size_t) binStride, 0.0f);
        synMag[ch].assign ((size_t) maxBins, 0.0f);
        synFreq[ch].assign ((size_t) maxBins, 0.0f);
        sumPhase[ch].assign ((size_t) maxBins, 0.0f);
    }

    panGainL.assign ((size_t) maxBins, 0.0f);
    panGainR.assign ((size_t) maxBins, 0.0f);
    fbGainBin.assign ((size_t) maxBins, 0.0f);
    tilt.assign ((size_t) maxBins, 1.0f);
    melPos.assign ((size_t) maxBins, 0.0f);
    prevMag.assign ((size_t) maxBins, 0.0f);

    const int block = juce::jlimit (64, kMaxBlockSize, maxBlockSize);
    for (int ch = 0; ch < 2; ++ch)
    {
        inScratch[ch].assign ((size_t) block, 0.0f);
        wetScratch[ch].assign ((size_t) block, 0.0f);
    }

    const int capacity = juce::nextPowerOfTwo (latency + block + 4);
    dryMask = capacity - 1;
    for (int ch = 0; ch < 2; ++ch) dryLine[ch].assign ((size_t) capacity, 0.0f);

    seq.prepare (sr);
    blendSmooth.prepare (sr, 20.0f);
    tapSmooth.prepare (sr, 20.0f);

    activeFragments = juce::jlimit (1, kMaxFractureFragments, activeFragments);
    rebuildBands (maxBins);
    rebuildScatter (seqSettings.seed);

    reset();
}

void FractureEngine::reset()
{
    stft.reset();
    seq.reset (seqSettings.seed);

    for (int ch = 0; ch < 2; ++ch)
    {
        std::fill (ring[ch].begin(), ring[ch].end(), 0.0f);
        std::fill (wetSpec[ch].begin(), wetSpec[ch].end(), 0.0f);
        std::fill (synMag[ch].begin(), synMag[ch].end(), 0.0f);
        std::fill (synFreq[ch].begin(), synFreq[ch].end(), 0.0f);
        std::fill (sumPhase[ch].begin(), sumPhase[ch].end(), 0.0f);
        std::fill (dryLine[ch].begin(), dryLine[ch].end(), 0.0f);
        std::fill (wetScratch[ch].begin(), wetScratch[ch].end(), 0.0f);
    }
    std::fill (prevMag.begin(), prevMag.end(), 0.0f);

    fragGain.fill (0.0f);  fragGainTarget.fill (0.0f);
    fragPitch.fill (0.0f); fragPitchTarget.fill (0.0f);
    fragPan.fill (0.0f);   fragPanTarget.fill (0.0f);
    fragFb.fill (0.0f);    fragFbTarget.fill (0.0f);
    fragDelay.fill (0.0f); fragDelayTarget.fill (0.0f);
    evolvePitch.fill (0.0f); evolveDelay.fill (0.0f); evolvePan.fill (0.0f); evolveGain.fill (0.0f);
    targetPitch.fill (0.0f); targetDelay.fill (0.0f); targetPan.fill (0.0f); targetGain.fill (0.0f);
    transientEnv.fill (0.0f);
    fragEnergy.fill (0.0f);
    probPass.fill (true);

    writeFrame = 0;
    dryWrite = 0;
    onsetFast = onsetSlow = 0.0f;
    wetTrim = 1.0f;
    framesSinceOnset = 1000;
    wasPlaying = false;
    blendSmooth.reset (0.0f);
    tapSmooth.reset (0.0f);
    reportedLatency.store (0, std::memory_order_relaxed);
    activityValue.store (0.0f, std::memory_order_relaxed);
    for (int f = 0; f < kMaxFractureFragments; ++f)
    {
        actGain[(size_t) f].store (0.0f, std::memory_order_relaxed);
        actEnergy[(size_t) f].store (0.0f, std::memory_order_relaxed);
    }
}

//==============================================================================
void FractureEngine::rebuildBands (int bins)
{
    const float nyquist = (float) (sr * 0.5);
    const float melMax = juce::jmax (1.0f, melOf (nyquist));
    const float binHz = (float) sr / (float) stft.size();

    for (int j = 0; j < bins; ++j)
        melPos[(size_t) j] = juce::jlimit (0.0f, 1.0f, melOf ((float) j * binHz) / melMax);

    const int frags = juce::jlimit (1, kMaxFractureFragments, activeFragments);
    bandEdge[0] = 0;
    for (int f = 1; f < frags; ++f)
    {
        const float hz = melToHz (melMax * (float) f / (float) frags);
        const int wanted = (int) std::lround (hz / binHz);
        bandEdge[(size_t) f] = juce::jlimit (bandEdge[(size_t) (f - 1)] + 1, bins - (frags - f), wanted);
    }
    for (int f = frags; f <= kMaxFractureFragments; ++f) bandEdge[(size_t) f] = bins;

    bandsBuiltFor = frags;
    tiltBuiltFor = -1.0f;   // melPos changed, so the tilt table must follow
}

void FractureEngine::rebuildScatter (uint32_t seed)
{
    scatterSeed = seed;
    Rng r (seed);
    for (int f = 0; f < kMaxFractureFragments; ++f)
    {
        scatterPitch[(size_t) f] = r.nextBipolar();
        scatterDelay[(size_t) f] = r.nextFloat();
        scatterPan[(size_t) f]   = r.nextBipolar();
    }
}

void FractureEngine::rebuildTilt (float tone, int bins)
{
    if (std::abs (tone - tiltBuiltFor) < 1.0e-4f) return;
    tiltBuiltFor = tone;
    const float t = (tone - 0.5f) * 2.0f;
    for (int j = 0; j < bins; ++j)
        tilt[(size_t) j] = std::exp2 (t * 2.0f * (melPos[(size_t) j] - 0.5f));
}

//==============================================================================
void FractureEngine::onNewStep()
{
    // Per-fragment probability gate — deterministic for a given seed and step index.
    Rng r (seq.stepSeed());
    for (int f = 0; f < kMaxFractureFragments; ++f)
        probPass[(size_t) f] = r.chance (juce::jlimit (0.0f, 1.0f, table->fragments[(size_t) f].probability));

    // EVOLVE: pick the next morph targets.
    Rng e (hashSeed (bp.seed, 0x51ED0000u ^ seq.stepCount()));
    for (int f = 0; f < kMaxFractureFragments; ++f)
    {
        targetPitch[(size_t) f] = e.nextBipolar();
        targetDelay[(size_t) f] = e.nextBipolar();
        targetPan[(size_t) f]   = e.nextBipolar();
        targetGain[(size_t) f]  = e.nextBipolar();
    }

    actStep.store (seq.stepIndex(), std::memory_order_relaxed);
}

void FractureEngine::detectOnset (const float* specL, const float* specR, int bins)
{
    const float hopSec = (float) ((double) stft.hop() / sr);
    const float envDecay = std::exp (-hopSec / (0.05f + bp.decay * 1.5f));

    double flux = 0.0;
    for (int j = 1; j < bins; ++j)
    {
        const int i0 = 2 * j, i1 = i0 + 1;
        const float mL = std::sqrt (specL[i0] * specL[i0] + specL[i1] * specL[i1]);
        const float mR = std::sqrt (specR[i0] * specR[i0] + specR[i1] * specR[i1]);
        const float m = 0.5f * (mL + mR);
        const float d = m - prevMag[(size_t) j];
        if (d > 0.0f) flux += (double) d;
        prevMag[(size_t) j] = m;
    }

    const float norm = 1.0f / juce::jmax (1.0f, 0.25f * (float) stft.size());
    const float f = std::isfinite ((float) flux) ? (float) flux * norm : 0.0f;

    onsetFast += (f - onsetFast) * 0.6f;
    onsetSlow += (f - onsetSlow) * 0.04f;
    ++framesSinceOnset;

    for (int i = 0; i < kMaxFractureFragments; ++i) transientEnv[(size_t) i] *= envDecay;

    const bool onset = framesSinceOnset > 3 && onsetFast > onsetSlow * 1.8f + 0.004f;
    if (! onset) return;

    framesSinceOnset = 0;

    if (bp.mode == FractureMode::Transient)
    {
        // Every onset shatters the spectrum a different way, reproducibly for a given seed.
        rebuildScatter (hashSeed (bp.seed, 0x7A110000u ^ (uint32_t) seq.stepCount()
                                                       ^ ((uint32_t) writeFrame * 2654435761u)));
        Rng r (scatterSeed);
        for (int i = 0; i < kMaxFractureFragments; ++i)
            transientEnv[(size_t) i] = r.chance (juce::jlimit (0.0f, 1.0f,
                                                   table->fragments[(size_t) i].probability)) ? 1.0f : 0.0f;
    }
}

//==============================================================================
void FractureEngine::updateFragments()
{
    const int frags = activeFragments;
    const auto& st = seq.state();
    const float hopSec = (float) ((double) stft.hop() / sr);

    const float seqWeight = juce::jlimit (0.0f, 1.0f, bp.mode == FractureMode::Rhythmic
                                                        ? juce::jmax (bp.sequence, 0.4f) : bp.sequence);
    const float evolveDepth = juce::jlimit (0.0f, 1.0f, bp.mode == FractureMode::Evolve
                                                          ? juce::jmax (bp.evolve, 0.5f) : bp.evolve);
    const float delayScaleSec = kMaxDelaySeconds * bp.delayScale;

    // Per-step decay envelope for RHYTHMIC gating.
    float stepEnv = 1.0f;
    if (bp.mode == FractureMode::Rhythmic)
        stepEnv = std::exp (-seq.phase() * (0.5f + 8.0f * (1.0f - bp.decay)));

    // ~8 ms one-pole evaluated once per hop; the evolve morph spans half a step.
    const float smooth = 1.0f - std::exp (-hopSec / 0.008f);
    const float morph = juce::jlimit (0.01f, 1.0f,
                                      (float) ((double) stft.hop()
                                               / juce::jmax (1.0, seq.stepLengthSamples() * 0.5)));

    for (int f = 0; f < frags; ++f)
    {
        const size_t i = (size_t) f;
        const auto& fr = table->fragments[i];
        const float fs = juce::jlimit (0.0f, 1.0f, fr.spread) * bp.spread;

        evolvePitch[i] += (targetPitch[i] - evolvePitch[i]) * morph;
        evolveDelay[i] += (targetDelay[i] - evolveDelay[i]) * morph;
        evolvePan[i]   += (targetPan[i]   - evolvePan[i])   * morph;
        evolveGain[i]  += (targetGain[i]  - evolveGain[i])  * morph;

        const bool maskOn = ((st.mask >> (f & 31)) & 1u) != 0;
        const float seqGate = (st.active && maskOn) ? juce::jlimit (0.0f, 2.0f, st.gate * st.gain) : 0.0f;

        // SPECTRAL is a static mode: the sequencer colours it but only half-gates it.
        const float gateWeight = bp.mode == FractureMode::Spectral ? seqWeight * 0.5f : seqWeight;
        float gate = 1.0f + (seqGate - 1.0f) * gateWeight;
        if (bp.mode == FractureMode::Rhythmic)  gate *= stepEnv;
        if (bp.mode == FractureMode::Transient) gate = transientEnv[i];
        if (! probPass[i]) gate = 0.0f;

        // The table's own pitch and the global offset are exact user values; every
        // scattered / sequenced / drifting contribution snaps to a consonant interval
        // so nothing lands a fraction of a semitone away and beats.
        const float scattered = quantiseInterval (fs * scatterPitch[i] * 24.0f)
                              + quantiseInterval (seqWeight * st.pitch
                                                  + evolveDepth * evolvePitch[i] * 24.0f);
        const float pitch = juce::jlimit (-48.0f, 48.0f, fr.pitch + bp.pitch + scattered);

        const float dNorm = juce::jlimit (0.0f, 1.0f, fr.delay + fs * scatterDelay[i] * 0.6f
                                                      + evolveDepth * evolveDelay[i] * 0.4f);
        const float pan = juce::jlimit (-1.0f, 1.0f, fr.pan + fs * scatterPan[i]
                                                     + seqWeight * st.pan + evolveDepth * evolvePan[i]);
        const float gainMul = juce::jlimit (0.0f, 2.0f,
                                            fr.gain * (1.0f + evolveDepth * evolveGain[i] * 0.5f));

        float frames = dNorm * delayScaleSec * (float) sr / (float) stft.hop();
        const float fbAmount = juce::jlimit (0.0f, 1.0f, fr.feedback) * bp.feedback;
        if (fbAmount > 0.01f) frames = juce::jmax (1.0f, frames);
        frames = juce::jlimit (0.0f, (float) (maxFrames - 3), frames);

        const float loopSec = juce::jmax (hopSec, frames * hopSec);
        const float decayTime = 0.08f + juce::jlimit (0.0f, 1.0f, fr.decay) * bp.decay * 7.0f;
        const float decayFactor = std::pow (0.001f, loopSec / juce::jmax (0.02f, decayTime));

        fragGainTarget[i]  = gate * gainMul;
        fragPitchTarget[i] = pitch;
        fragPanTarget[i]   = pan;
        fragFbTarget[i]    = juce::jlimit (0.0f, kMaxFeedback, fbAmount * kMaxFeedback * decayFactor);
        fragDelayTarget[i] = frames;

        fragGain[i]  += (fragGainTarget[i]  - fragGain[i])  * smooth;
        fragPitch[i] += (fragPitchTarget[i] - fragPitch[i]) * smooth;
        fragPan[i]   += (fragPanTarget[i]   - fragPan[i])   * smooth;
        fragFb[i]    += (fragFbTarget[i]    - fragFb[i])    * smooth;
        fragDelay[i] += juce::jlimit (-1.0f, 1.0f, fragDelayTarget[i] - fragDelay[i]);
    }

    // Feedback makeup. A delay loop of gain g settles at 1/sqrt(1 - g²) times the
    // input energy; trimming by the inverse keeps the wet level roughly constant as
    // the feedback control is opened, instead of driving the master limiter.
    float sumFb = 0.0f;
    int voiced = 0;
    for (int f = 0; f < frags; ++f)
        if (fragGain[(size_t) f] > 1.0e-4f) { sumFb += fragFb[(size_t) f]; ++voiced; }

    const float g = voiced > 0 ? juce::jlimit (0.0f, 0.95f, sumFb / (float) voiced) : 0.0f;
    wetTrim += (std::sqrt (juce::jmax (0.05f, 1.0f - g * g)) - wetTrim) * smooth;
}

//==============================================================================
void FractureEngine::processFrame (float* specL, float* specR, int bins)
{
    if (seq.advance (stft.hop(), seqSettings, *table)) onNewStep();
    detectOnset (specL, specR, bins);
    updateFragments();

    const size_t stride = (size_t) binStride;
    const size_t nFloats = (size_t) (2 * bins);

    float* ringL = ring[0].data() + (size_t) writeFrame * stride;
    float* ringR = ring[1].data() + (size_t) writeFrame * stride;
    std::memcpy (ringL, specL, sizeof (float) * nFloats);
    std::memcpy (ringR, specR, sizeof (float) * nFloats);

    float* wl = wetSpec[0].data();
    float* wr = wetSpec[1].data();
    std::fill (wl, wl + nFloats, 0.0f);
    std::fill (wr, wr + nFloats, 0.0f);
    std::fill (panGainL.begin(), panGainL.begin() + bins, 0.0f);
    std::fill (panGainR.begin(), panGainR.begin() + bins, 0.0f);
    std::fill (fbGainBin.begin(), fbGainBin.begin() + bins, 0.0f);
    for (int ch = 0; ch < 2; ++ch)
    {
        std::fill (synMag[ch].begin(), synMag[ch].begin() + bins, 0.0f);
        for (int j = 0; j < bins; ++j) synFreq[ch][(size_t) j] = (float) j;
    }

    bool anyShift = false;
    const int frags = activeFragments;

    for (int f = 0; f < frags; ++f)
    {
        const size_t i = (size_t) f;
        const int lo = bandEdge[i], hi = bandEdge[i + 1];
        const float g = fragGain[i];
        if (hi <= lo || g <= 1.0e-5f) { fragEnergy[i] = 0.0f; continue; }

        const int d = (int) std::lround (fragDelay[i]);
        const float ratio = std::exp2 (fragPitch[i] * (1.0f / 12.0f));
        const float theta = (fragPan[i] + 1.0f) * (0.5f * kHalfPi);
        const float pl = 1.41421356f * std::cos (theta);   // unity at centre, constant power
        const float pr = 1.41421356f * std::sin (theta);
        const float fb = fragFb[i];

        const float* sL = ring[0].data() + (size_t) ringIndex (writeFrame - d) * stride;
        const float* sR = ring[1].data() + (size_t) ringIndex (writeFrame - d) * stride;
        double energy = 0.0;

        if (std::abs (ratio - 1.0f) < 1.0e-4f)
        {
            // Unity pitch: an exact complex copy. Delaying whole frames and
            // overlap-adding reconstructs a clean delayed copy of the band.
            for (int k = lo; k < hi; ++k)
            {
                const int i0 = 2 * k, i1 = i0 + 1;
                const float aL = sL[i0] * g, bL = sL[i1] * g;
                const float aR = sR[i0] * g, bR = sR[i1] * g;
                wl[i0] += aL; wl[i1] += bL;
                wr[i0] += aR; wr[i1] += bR;
                panGainL[(size_t) k] = pl; panGainR[(size_t) k] = pr; fbGainBin[(size_t) k] = fb;
                energy += (double) (aL * aL + bL * bL + aR * aR + bR * bR);
            }
        }
        else
        {
            // Phase-vocoder shift: instantaneous frequency from the delayed frame and
            // its predecessor, scattered to the target bins, integrated at synthesis.
            anyShift = true;
            const float* pL = ring[0].data() + (size_t) ringIndex (writeFrame - d - 1) * stride;
            const float* pR = ring[1].data() + (size_t) ringIndex (writeFrame - d - 1) * stride;

            for (int k = lo; k < hi; ++k)
            {
                const int target = (int) std::lround ((float) k * ratio);
                if (target < 0 || target >= bins) continue;

                const int i0 = 2 * k, i1 = i0 + 1;
                const float expected = (float) k * radiansPerBin;
                bool wrote = false;

                for (int ch = 0; ch < 2; ++ch)
                {
                    const float* src = ch == 0 ? sL : sR;
                    const float* prv = ch == 0 ? pL : pR;
                    const float re = src[i0], im = src[i1];
                    const float mag = std::sqrt (re * re + im * im) * g;
                    if (mag <= 1.0e-9f) continue;

                    const float dphi = wrapPi (std::atan2 (im, re)
                                               - std::atan2 (prv[i1], prv[i0]) - expected);
                    synMag[ch][(size_t) target] += mag;
                    synFreq[ch][(size_t) target] = ((float) k + dphi * binsPerRadian) * ratio;
                    energy += (double) mag * (double) mag;
                    wrote = true;
                }

                if (wrote)
                {
                    panGainL[(size_t) target] = pl;
                    panGainR[(size_t) target] = pr;
                    fbGainBin[(size_t) target] = fb;
                }
            }
        }

        fragEnergy[i] = (float) energy;
    }

    // --- phase-vocoder synthesis --------------------------------------------
    for (int ch = 0; ch < 2; ++ch)
    {
        float* sp = sumPhase[ch].data();
        const float* sm = synMag[ch].data();
        const float* sf = synFreq[ch].data();
        float* w = ch == 0 ? wl : wr;

        if (anyShift)
        {
            for (int j = 0; j < bins; ++j)
            {
                sp[j] = wrapPi (sp[j] + sf[j] * radiansPerBin);
                const float m = sm[j];
                if (m > 0.0f)
                {
                    w[2 * j]     += m * std::cos (sp[j]);
                    w[2 * j + 1] += m * std::sin (sp[j]);
                }
            }
        }
        else
        {
            // Keep the synthesis phases running at their nominal rate so a fragment
            // that starts shifting later begins from a coherent phase.
            for (int j = 0; j < bins; ++j) sp[j] = wrapPi (sp[j] + (float) j * radiansPerBin);
        }
    }

    // --- feedback into the delay line, with an absolute energy bound ----------
    double ringEnergy = 0.0;
    for (int j = 0; j < bins; ++j)
    {
        const int i0 = 2 * j, i1 = i0 + 1;
        const float fbg = fbGainBin[(size_t) j];
        if (fbg > 0.0f)
        {
            ringL[i0] += wl[i0] * fbg; ringL[i1] += wl[i1] * fbg;
            ringR[i0] += wr[i0] * fbg; ringR[i1] += wr[i1] * fbg;
        }
        ringEnergy += (double) ringL[i0] * ringL[i0] + (double) ringL[i1] * ringL[i1]
                    + (double) ringR[i0] * ringR[i0] + (double) ringR[i1] * ringR[i1];
    }

    const double n2 = (double) stft.size() * (double) stft.size();
    if (! std::isfinite (ringEnergy))
    {
        std::fill (ringL, ringL + nFloats, 0.0f);
        std::fill (ringR, ringR + nFloats, 0.0f);
        std::fill (wl, wl + nFloats, 0.0f);
        std::fill (wr, wr + nFloats, 0.0f);
        for (int ch = 0; ch < 2; ++ch)
            std::fill (sumPhase[ch].begin(), sumPhase[ch].end(), 0.0f);
        if (safety != nullptr) safety->note (SafetyEvent::NaN, Subsystem::Fracture);
    }
    else
    {
        const float level = (float) std::sqrt (ringEnergy / juce::jmax (1.0e-12, n2 * 0.1875));
        if (level > kRingCeiling)
        {
            const float scale = kRingCeiling / level;
            juce::FloatVectorOperations::multiply (ringL, scale, (int) nFloats);
            juce::FloatVectorOperations::multiply (ringR, scale, (int) nFloats);
            if (safety != nullptr) safety->note (SafetyEvent::FeedbackClamp, Subsystem::Fracture);
        }
    }

    // --- spectral tilt + pan → output spectra ---------------------------------
    for (int j = 0; j < bins; ++j)
    {
        const int i0 = 2 * j, i1 = i0 + 1;
        const float t = tilt[(size_t) j] * wetTrim;
        const float gl = t * panGainL[(size_t) j];
        const float gr = t * panGainR[(size_t) j];
        specL[i0] = wl[i0] * gl; specL[i1] = wl[i1] * gl;
        specR[i0] = wr[i0] * gr; specR[i1] = wr[i1] * gr;
    }

    writeFrame = ringIndex (writeFrame + 1);

    // --- diagnostics ----------------------------------------------------------
    float peak = 1.0e-9f;
    for (int f = 0; f < frags; ++f) peak = juce::jmax (peak, fragEnergy[(size_t) f]);
    for (int f = 0; f < kMaxFractureFragments; ++f)
    {
        const size_t i = (size_t) f;
        actGain[i].store (f < frags ? juce::jlimit (0.0f, 1.0f, fragGain[i]) : 0.0f,
                          std::memory_order_relaxed);
        actEnergy[i].store (f < frags ? juce::jlimit (0.0f, 1.0f, fragEnergy[i] / peak) : 0.0f,
                            std::memory_order_relaxed);
    }
    actFragments.store (frags, std::memory_order_relaxed);
}

//==============================================================================
void FractureEngine::process (float* l, float* r, int n, const RenderContext& ctx)
{
    if (n <= 0 || l == nullptr || r == nullptr) return;

    if (auto* fresh = handoff.acquire()) table = fresh;
    safety = ctx.diagnostics != nullptr ? &ctx.diagnostics->safety : nullptr;

    const bool on      = ctx.flag (Param::fractureOn);
    const float amount = clamp01 (ctx.param (Param::fractureAmount));
    const float mix    = clamp01 (ctx.param (Param::fractureMix));
    const float tone   = clamp01 (ctx.param (Param::fractureTone));

    bp.mode        = (FractureMode) juce::jlimit (0, (int) FractureMode::Count - 1,
                                                  ctx.choice (Param::fractureMode));
    bp.spread      = clamp01 (ctx.param (Param::fractureSpread));
    bp.sequence    = clamp01 (ctx.param (Param::fractureSequence));
    bp.feedback    = clamp01 (ctx.param (Param::fractureFeedback));
    bp.pitch       = juce::jlimit (-24.0f, 24.0f, ctx.param (Param::fracturePitch));
    bp.delayScale  = clamp01 (ctx.param (Param::fractureDelay));
    bp.decay       = clamp01 (ctx.param (Param::fractureDecay));
    bp.evolve      = clamp01 (ctx.param (Param::fractureEvolve));
    bp.probability = clamp01 (ctx.param (Param::fractureProbability));
    bp.seed        = (uint32_t) juce::jlimit (0, 9999, ctx.choice (Param::fractureSeed));

    const int fragChoice = juce::jlimit (0, 2, ctx.choice (Param::fractureFragments));
    bp.fragments = fragChoice == 0 ? 8 : (fragChoice == 1 ? 16 : 32);

    seqSettings.numSteps     = juce::jlimit (1, kMaxSequencerSteps, ctx.choice (Param::fractureSteps));
    seqSettings.sync         = ctx.flag (Param::fractureSync);
    seqSettings.division     = juce::jlimit (0, 11, ctx.choice (Param::fractureDivision));
    seqSettings.rateHz       = juce::jlimit (0.01f, 200.0f, ctx.param (Param::fractureRate));
    seqSettings.swing        = clamp01 (ctx.param (Param::fractureSwing));
    seqSettings.direction    = (FractureDirection) juce::jlimit (0, (int) FractureDirection::Count - 1,
                                                                 ctx.choice (Param::fractureDirection));
    seqSettings.probability  = bp.probability;
    seqSettings.randomAmount = clamp01 (ctx.param (Param::fractureRandom));
    seqSettings.seed         = bp.seed;
    seqSettings.bpm          = ctx.transport.bpm > 1.0 ? ctx.transport.bpm : 120.0;

    if (bandsBuiltFor != bp.fragments)
    {
        activeFragments = bp.fragments;
        rebuildBands (stft.numBins());
    }
    if (bp.mode != FractureMode::Transient && bp.seed != scatterSeed) rebuildScatter (bp.seed);
    rebuildTilt (tone, stft.numBins());

    // Retrigger: transport start edge, or an explicit noteStarted() from the principal.
    const bool noteRequest = retrigRequest.exchange (false, std::memory_order_acq_rel);
    const bool playing = ctx.transport.isPlaying;
    if (ctx.flag (Param::fractureRetrig) && ((playing && ! wasPlaying) || noteRequest))
        seq.reset (bp.seed);
    wasPlaying = playing;

    const float blendTarget = on ? amount * mix : 0.0f;
    blendSmooth.setTarget (blendTarget);
    const bool engaged = blendTarget > 0.0f || blendSmooth.getCurrent() > 1.0e-6f;
    tapSmooth.setTarget (engaged ? 1.0f : 0.0f);
    const bool active = engaged || tapSmooth.getCurrent() > 1.0e-6f;

    // Latency is zero while disengaged so an untouched patch adds none at all.
    const int wantLatency = engaged ? latency : 0;
    if (reportedLatency.exchange (wantLatency, std::memory_order_relaxed) != wantLatency)
        latencyDirty.store (true, std::memory_order_release);

    const int capacity = (int) inScratch[0].size();
    double wetEnergy = 0.0, dryEnergy = 0.0;

    for (int done = 0; done < n;)
    {
        const int m = juce::jmin (capacity, n - done);
        float* il = inScratch[0].data();
        float* ir = inScratch[1].data();
        std::memcpy (il, l + done, sizeof (float) * (size_t) m);
        std::memcpy (ir, r + done, sizeof (float) * (size_t) m);

        // Dry path: crossfade between the live tap (latency 0, bit-exact when
        // disengaged) and a tap delayed to match the STFT (aligned when engaged).
        const float x0 = tapSmooth.getCurrent();
        const float x1 = tapSmooth.skip (m);
        if (! tapSmooth.isSmoothing()) tapSmooth.reset (tapSmooth.getTarget());
        const float xStep = (x1 - x0) / (float) m;
        float x = x0;

        float* dryL = dryLine[0].data();
        float* dryR = dryLine[1].data();
        for (int i = 0; i < m; ++i)
        {
            dryL[dryWrite] = il[i];
            dryR[dryWrite] = ir[i];
            const int readPos = (dryWrite - latency) & dryMask;
            l[done + i] = il[i] + (dryL[readPos] - il[i]) * x;
            r[done + i] = ir[i] + (dryR[readPos] - ir[i]) * x;
            dryWrite = (dryWrite + 1) & dryMask;
            x += xStep;
        }

        float* wetL = wetScratch[0].data();
        float* wetR = wetScratch[1].data();
        stft.process (il, ir, wetL, wetR, m, active,
                      [this] (float* sl, float* sr2, int bins) { processFrame (sl, sr2, bins); });

        const float b0 = blendSmooth.getCurrent();
        const float b1 = blendSmooth.skip (m);
        if (! blendSmooth.isSmoothing()) blendSmooth.reset (blendSmooth.getTarget());

        const float dg0 = std::cos (b0 * kHalfPi), wg0 = std::sin (b0 * kHalfPi);
        const float dg1 = std::cos (b1 * kHalfPi), wg1 = std::sin (b1 * kHalfPi);
        const float dStep = (dg1 - dg0) / (float) m;
        const float wStep = (wg1 - wg0) / (float) m;

        float dg = dg0, wg = wg0;
        int nonFinite = 0;
        for (int i = 0; i < m; ++i)
        {
            float a = wetL[i], b = wetR[i];
            if (! (std::isfinite (a) && std::isfinite (b))) { a = 0.0f; b = 0.0f; ++nonFinite; }

            const float outWetL = a * wg, outWetR = b * wg;
            const float outDryL = l[done + i] * dg, outDryR = r[done + i] * dg;

            l[done + i] = outDryL + outWetL;
            r[done + i] = outDryR + outWetR;

            wetEnergy += (double) outWetL * outWetL + (double) outWetR * outWetR;
            dryEnergy += (double) outDryL * outDryL + (double) outDryR * outDryR;

            dg += dStep; wg += wStep;
        }

        if (nonFinite > 0)
        {
            if (safety != nullptr) safety->note (SafetyEvent::NaN, Subsystem::Fracture, -1, nonFinite);
            stft.reset();
            for (int ch = 0; ch < 2; ++ch)
            {
                std::fill (ring[ch].begin(), ring[ch].end(), 0.0f);
                std::fill (sumPhase[ch].begin(), sumPhase[ch].end(), 0.0f);
            }
        }

        done += m;
    }

    const double total = wetEnergy + dryEnergy;
    const float wetShare = total > 1.0e-12 ? (float) (wetEnergy / total) : 0.0f;
    const float previous = activityValue.load (std::memory_order_relaxed);
    activityValue.store (previous + (wetShare - previous) * 0.25f, std::memory_order_relaxed);
}

//==============================================================================
void FractureEngine::fillFragmentActivity (FragmentActivity& out) const noexcept
{
    out.numFragments = actFragments.load (std::memory_order_relaxed);
    out.currentStep  = actStep.load (std::memory_order_relaxed);
    out.overall      = activityValue.load (std::memory_order_relaxed);
    for (int f = 0; f < kMaxFractureFragments; ++f)
    {
        out.gain[(size_t) f]   = actGain[(size_t) f].load (std::memory_order_relaxed);
        out.energy[(size_t) f] = actEnergy[(size_t) f].load (std::memory_order_relaxed);
    }
}

} // namespace am
