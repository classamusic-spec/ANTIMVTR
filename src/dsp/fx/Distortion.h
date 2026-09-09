#pragma once

#include "FXCommon.h"

#include <juce_dsp/juce_dsp.h>

namespace am::fx
{

/**
    DISTORTION / SATURATION — five voices of dirt.

        SOFT   symmetric tanh, the polite one
        TUBE   asymmetric triode-ish curve (even + odd harmonics)
        FOLD   sine wavefolder, metallic and unpredictable
        CRUSH  bit + sample-rate reduction (deliberately aliased)
        BOTH   TAPE: pre/de-emphasis around a soft knee with HF loss

    Everything except CRUSH runs 2x oversampled through JUCE's polyphase IIR
    halfband pair, so the harmonics stay where they belong. The dry/wet blend
    happens inside the oversampled domain, which keeps both paths on exactly
    the same filter delay (no comb filtering when mix < 1).
*/
class Distortion
{
public:
    enum Mode { Soft = 0, Tube, Fold, Crush, Tape, NumModes };

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxBlock = maxBlockSize;

        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversampler->initProcessing ((size_t) maxBlockSize);

        drive.prepare (sampleRate, 20.0f);
        mix.prepare (sampleRate, 20.0f);
        for (auto& d : dcBlock) d.prepare (sampleRate, 12.0f);
        for (auto& f : tapeLoss) { f.prepare (sampleRate * 2.0); f.setCutoff (11000.0f); }
        reset();
    }

    void reset()
    {
        if (oversampler != nullptr) oversampler->reset();
        for (auto& d : dcBlock) d.reset();
        for (auto& f : tapeLoss) f.reset();
        for (auto& h : holdValue) h = 0.0f;
        for (auto& c : holdCount) c = 0;
        fade = 1.0f;
        previousMode = mode;
    }

    void setParams (int newMode, float drive01, float mix01) noexcept
    {
        newMode = juce::jlimit (0, (int) NumModes - 1, newMode);
        if (newMode != mode)
        {
            previousMode = mode;
            mode = newMode;
            fade = 0.0f;               // crossfade the transfer curves over ~5 ms
        }
        drive.setTarget (clampf (drive01, 0.0f, 1.0f));
        mix.setTarget (clampf (mix01, 0.0f, 1.0f));
    }

    void process (float* l, float* r, int n) noexcept
    {
        if (mode == Crush && previousMode == Crush)
        {
            processBase (l, r, n);
            return;
        }

        float* channels[2] { l, r };
        juce::dsp::AudioBlock<float> block (channels, 2, (size_t) n);
        auto up = oversampler->processSamplesUp (block);

        float* ul = up.getChannelPointer (0);
        float* ur = up.getChannelPointer (1);
        const int un = (int) up.getNumSamples();
        const float fadeStep = 1.0f / std::max (1.0f, (float) (sr * 2.0 * 0.005));

        for (int i = 0; i < un; ++i)
        {
            const float d = i % 2 == 0 ? drive.next() : drive.current();
            const float m = i % 2 == 0 ? mix.next() : mix.current();
            const float pre = expMap (d, 1.0f, 34.0f);
            const float comp = clampf (std::pow (pre, -0.55f), 0.04f, 1.0f);
            if (fade < 1.0f) fade = std::min (1.0f, fade + fadeStep);

            for (int c = 0; c < 2; ++c)
            {
                float* buf = c == 0 ? ul : ur;
                const float x = buf[i];
                float wet = shape (mode, c, x, pre, comp, d);
                if (fade < 1.0f)
                    wet = wet * fade + shape (previousMode, c, x, pre, comp, d) * (1.0f - fade);
                buf[i] = x + m * (wet - x);
            }
        }

        oversampler->processSamplesDown (block);
        for (int c = 0; c < 2; ++c)
        {
            float* buf = c == 0 ? l : r;
            for (int i = 0; i < n; ++i) buf[i] = dcBlock[(size_t) c].process (buf[i]);
        }
    }

    /** Static transfer curve, exposed so tests can measure THD without a graph. */
    static float curve (int mode, float x, float pre) noexcept
    {
        switch (mode)
        {
            case Tube:  { const float t = x * pre; return t >= 0.0f ? fastTanh (t) : 0.82f * fastTanh (t * 0.78f); }
            case Fold:  return std::sin (clampf (x * pre * 0.85f, -40.0f, 40.0f));
            case Tape:  { const float t = x * pre * 0.8f; return 1.12f * t / (1.0f + std::abs (t)); }
            case Soft:
            default:    return fastTanh (x * pre);
        }
    }

private:
    inline float shape (int m, int channel, float x, float pre, float comp, float drive01) noexcept
    {
        if (m == Crush) return crush (channel, x, drive01);
        float y = curve (m, x, pre) * comp;
        if (m == Tape) y = tapeLoss[(size_t) channel].lp (y);
        return y;
    }

    inline float crush (int channel, float x, float drive01) noexcept
    {
        const float bits = 16.0f - 13.0f * drive01;
        const float step = std::pow (2.0f, 1.0f - bits);
        const int hold = 1 + (int) (drive01 * drive01 * 24.0f);
        auto& counter = holdCount[(size_t) channel];
        auto& held = holdValue[(size_t) channel];
        if (counter <= 0)
        {
            held = step * std::round (clampf (x, -1.5f, 1.5f) / step);
            counter = hold;
        }
        --counter;
        return held;
    }

    void processBase (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float d = drive.next();
            const float m = mix.next();
            l[i] = dcBlock[0].process (l[i] + m * (crush (0, l[i], d) - l[i]));
            r[i] = dcBlock[1].process (r[i] + m * (crush (1, r[i], d) - r[i]));
        }
    }

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    double sr = 48000.0;
    int maxBlock = 512;
    int mode = Soft, previousMode = Soft;
    float fade = 1.0f;
    SmoothParam drive, mix;
    DCBlocker dcBlock[2];
    OnePoleTPT tapeLoss[2];
    float holdValue[2] {};
    int holdCount[2] {};
};

} // namespace am::fx
