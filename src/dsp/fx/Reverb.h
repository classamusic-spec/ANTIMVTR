#pragma once

#include "FXCommon.h"
#include "PitchShifter.h"

namespace am::fx
{

/**
    ALGORITHMIC REVERB — 8-line feedback delay network.

    Signal path:

        pre-delay → 4 input allpasses per channel (diffusion)
                  → 8 modulated delay lines
                       · frequency dependent damping (one-pole LP)
                       · low-cut inside the loop (keeps tails clean)
                       · Jot decay gains  g = 10^(-3T/RT60)
                  → orthogonal 8x8 Hadamard mixing
                  → two orthogonal output taps (wide, decorrelated stereo)

    The Hadamard matrix is unitary and every line gain is strictly below one,
    so the network cannot grow; the modulation (a slow sine per line, read
    with Hermite interpolation) removes the metallic ring that fixed-length
    FDNs get on sustained tones.

    SHIMMER: an optional octave-up pitch shifter sits in the recirculation
    path, soft-clipped and band-limited, so the tail keeps climbing without
    ever running away.
*/
class Reverb
{
public:
    static constexpr int kLines = 8;
    static constexpr float kMaxLineGain = 0.9995f;
    static constexpr float kMaxPredelayMs = 260.0f;

    /** RT60 in seconds for a decay control and the FEEDBACK macro. */
    static float rt60Seconds (float decay01, float feedbackMacro) noexcept
    {
        const float base = expMap (clampf (decay01, 0.0f, 1.0f), 0.25f, 22.0f);
        const float scale = 0.45f + 1.5f * clampf (feedbackMacro, 0.0f, 1.0f);
        return clampf (base * scale, 0.08f, 45.0f);
    }

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        for (auto& p : predelay) p.prepare ((int) (sampleRate * (double) kMaxPredelayMs * 0.001) + 64);

        const int maxLine = (int) (sampleRate * 0.30) + 64;
        for (int i = 0; i < kLines; ++i)
        {
            lines[i].prepare (maxLine);
            damping[i].prepare (sampleRate);
            lowCut[i].prepare (sampleRate);
            modLfo[i].prepare (sampleRate);
            modLfo[i].reset (0.137f * (float) i);
            modLfo[i].setRate (kModRate[i]);
        }

        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < kInputStages; ++s)
                inputDiffusion[c][s].prepare ((int) (sampleRate * 0.05) + 8);

        shifter.prepare (sampleRate, 70.0f);
        shimmerHigh.prepare (sampleRate);
        shimmerHigh.setCutoff (260.0f);
        shimmerLow.prepare (sampleRate);
        shimmerLow.setCutoff (7000.0f);

        mix.prepare (sampleRate, 25.0f);
        predelaySamples.prepare (sampleRate, 120.0f);
        shimmerGain.prepare (sampleRate, 80.0f);
        for (int i = 0; i < kLines; ++i)
        {
            lineDelay[i].prepare (sampleRate, 150.0f);
            lineGain[i].prepare (sampleRate, 80.0f);
        }
        reset();
    }

    void reset()
    {
        for (auto& p : predelay) p.clear();
        for (int i = 0; i < kLines; ++i)
        {
            lines[i].clear();
            damping[i].reset();
            lowCut[i].reset();
        }
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < kInputStages; ++s)
                inputDiffusion[c][s].clear();
        shifter.reset();
        shimmerHigh.reset();
        shimmerLow.reset();
        shimmerState = 0.0f;
        clamped = false;
        first = true;
        // The line-modulation LFOs are state. Left where the last patch stopped them they detune the
        // first moments of the next one differently every time, so a reset has to put them back on
        // the spread `prepare` gives them.
        for (int i = 0; i < kLines; ++i)
            modLfo[i].reset (0.137f * (float) i);
        // ... and so are the smoothed controls. `mix` is the one that hurts: the reverb's output
        // is near zero while its lines refill, so a `mix` left on the old patch scales the DRY
        // signal through the module and the error is at full level from the first sample.
        mix.reset(); predelaySamples.reset(); shimmerGain.reset();
        for (int i = 0; i < kLines; ++i) { lineDelay[i].reset(); lineGain[i].reset(); }
    }

    /**
        size01/decay01/damp01/mod01/mix01 are the module's own controls;
        sizeScale, toneTilt (-1..1) and feedbackMacro come from the macros.
    */
    void setParams (float size01, float decay01, float damp01, float predelayMs, float mod01,
                    float mix01, float sizeScale, float toneTilt, float feedbackMacro,
                    float shimmerAmount, float shimmerSemitones) noexcept
    {
        const float signature[8] { size01, decay01, damp01, sizeScale, toneTilt, feedbackMacro, shimmerAmount, shimmerSemitones };
        bool unchanged = ! first;
        for (int i = 0; i < 8 && unchanged; ++i)
            unchanged = std::abs (signature[i] - cached[i]) < 1.0e-5f;

        // These three are cheap, so they always follow the parameter.
        modDepth = clampf (mod01, 0.0f, 1.0f) * 0.0032f * (float) sr;
        predelaySamples.setTarget (clampf (predelayMs, 0.0f, kMaxPredelayMs) * 0.001f * (float) sr);
        mix.setTarget (clampf (mix01, 0.0f, 1.0f));

        if (unchanged) return;                 // nothing that needs new coefficients moved
        for (int i = 0; i < 8; ++i) cached[i] = signature[i];

        const float room = clampf ((0.30f + 1.55f * clampf (size01, 0.0f, 1.0f)) * sizeScale, 0.12f, 2.6f);
        rt60 = rt60Seconds (decay01, feedbackMacro);
        clamped = false;

        for (int i = 0; i < kLines; ++i)
        {
            const float seconds = kLineMs[i] * room * 0.001f;
            const float samples = clampf (seconds * (float) sr, 16.0f, (float) lines[i].capacity() - 512.0f);
            lineDelay[i].setTarget (samples);

            const float t = samples / (float) sr;
            float g = std::pow (10.0f, -3.0f * t / rt60);
            if (g > kMaxLineGain) { g = kMaxLineGain; clamped = true; }
            lineGain[i].setTarget (g);

            // Damping: darker with DAMP, brighter with a positive TONE tilt.
            const float dampNorm = clampf (damp01 - 0.35f * toneTilt, 0.0f, 1.0f);
            damping[i].setCutoff (expMap (1.0f - dampNorm, 1100.0f, 17000.0f) * kLineTilt[i]);
            lowCut[i].setCutoff (clampf (28.0f + 120.0f * clampf (toneTilt, 0.0f, 1.0f), 20.0f, 220.0f));
        }

        diffusionScale = clampf (0.55f + 0.45f * room, 0.25f, 1.8f);
        shifter.setSemitones (shimmerSemitones);
        shimmerGain.setTarget (clampf (shimmerAmount, 0.0f, 1.0f) * 0.62f);

        if (first)
        {
            predelaySamples.reset (predelaySamples.target());
            for (int i = 0; i < kLines; ++i) { lineDelay[i].reset (lineDelay[i].target()); lineGain[i].reset (lineGain[i].target()); }
            shimmerGain.reset (shimmerGain.target());
            first = false;
        }
    }

    bool feedbackClamped() const noexcept { return clamped; }
    float currentRT60() const noexcept { return rt60; }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float dryL = l[i], dryR = r[i];
            const float m = mix.next();
            const float pd = predelaySamples.next();

            predelay[0].write (dryL);
            predelay[1].write (dryR);
            float inL = predelay[0].readLinear (std::max (1.0f, pd));
            float inR = predelay[1].readLinear (std::max (1.0f, pd));

            for (int s = 0; s < kInputStages; ++s)
            {
                inputDiffusion[0][s].set (kDiffuseMsL[s] * diffusionScale * 0.001f * (float) sr, 0.72f);
                inputDiffusion[1][s].set (kDiffuseMsR[s] * diffusionScale * 0.001f * (float) sr, 0.72f);
                inL = inputDiffusion[0][s].process (inL);
                inR = inputDiffusion[1][s].process (inR);
            }

            // --- read the network
            float v[kLines];
            for (int k = 0; k < kLines; ++k)
            {
                const float d = lineDelay[k].next() + modDepth * modLfo[k].next();
                float s = lines[k].readHermite (d);
                s = damping[k].lp (s);
                s = lowCut[k].hp (s);
                v[k] = s * lineGain[k].next();
            }

            // --- orthogonal outputs
            float outL = 0.0f, outR = 0.0f;
            for (int k = 0; k < kLines; ++k)
            {
                outL += v[k] * kTapL[k];
                outR += v[k] * kTapR[k];
            }
            outL *= kTapScale;
            outR *= kTapScale;

            // --- shimmer: octave-up regeneration
            const float shimmerAmt = shimmerGain.next();
            float shimmer = 0.0f;
            if (shimmerAmt > 1.0e-4f)
            {
                const float mono = 0.5f * (outL + outR);
                float s = shifter.process (mono + 0.6f * shimmerState);
                s = shimmerLow.lp (shimmerHigh.hp (s));
                shimmerState = softLimit (s, 0.8f, 1.1f);
                shimmer = shimmerState * shimmerAmt;
            }
            else
            {
                shimmerState *= 0.9f;
                shifter.process (0.0f);
            }

            // --- mix and feed back
            hadamard8 (v);
            for (int k = 0; k < kLines; ++k)
            {
                const float inject = (k < 4 ? inL * kInjectL[k] : inR * kInjectR[k - 4]) + shimmer * kInjectShimmer[k];
                lines[k].write (v[k] + inject);
            }

            l[i] = dryL + m * (outL - dryL);
            r[i] = dryR + m * (outR - dryR);
        }
    }

private:
    static constexpr int kInputStages = 4;
    static constexpr float kTapScale = 0.42f;

    // Mutually prime-ish line lengths (ms at room scale 1.0).
    static constexpr float kLineMs[kLines]   { 23.1f, 29.7f, 34.3f, 41.9f, 47.3f, 53.9f, 61.7f, 71.3f };
    static constexpr float kLineTilt[kLines] { 1.15f, 1.05f, 1.0f, 0.94f, 0.9f, 0.85f, 0.8f, 0.74f };
    static constexpr float kModRate[kLines]  { 0.093f, 0.131f, 0.172f, 0.211f, 0.257f, 0.311f, 0.371f, 0.433f };
    static constexpr float kTapL[kLines]     { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f };
    static constexpr float kTapR[kLines]     { 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f };
    static constexpr float kInjectL[4]       { 0.5f, 0.5f, -0.5f, 0.5f };
    static constexpr float kInjectR[4]       { 0.5f, -0.5f, 0.5f, -0.5f };
    static constexpr float kInjectShimmer[kLines] { 0.35f, -0.35f, 0.35f, 0.35f, -0.35f, 0.35f, -0.35f, 0.35f };
    static constexpr float kDiffuseMsL[kInputStages] { 4.77f, 3.59f, 12.73f, 9.31f };
    static constexpr float kDiffuseMsR[kInputStages] { 5.13f, 3.91f, 13.61f, 10.07f };

    double sr = 48000.0;
    DelayLine predelay[2];
    Allpass inputDiffusion[2][kInputStages];
    DelayLine lines[kLines];
    OnePoleTPT damping[kLines], lowCut[kLines];
    LFO modLfo[kLines];
    SmoothParam lineDelay[kLines], lineGain[kLines];
    SmoothParam mix, predelaySamples, shimmerGain;
    PitchShifter shifter;
    OnePoleTPT shimmerHigh, shimmerLow;
    float shimmerState = 0.0f;
    float cached[8] { -99.0f, -99.0f, -99.0f, -99.0f, -99.0f, -99.0f, -99.0f, -99.0f };
    float modDepth = 0.0f, diffusionScale = 1.0f, rt60 = 2.0f;
    bool clamped = false, first = true;
};

} // namespace am::fx
