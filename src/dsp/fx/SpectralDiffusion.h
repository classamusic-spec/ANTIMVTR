#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    SPECTRAL DIFFUSION — an allpass smearing network.

    Six modulated Schroeder allpasses per channel, with mutually prime delays
    and a different set for left and right. Because allpasses are flat in
    magnitude, this does not colour the tone: it disperses transients in time
    (a struck note turns into a wash) and decorrelates the two channels, which
    is what makes DUST and NEBULA feel like weather rather than a room.

    AMOUNT stretches the delays, raises the allpass coefficients and fades the
    network in, so at 0 the module is transparent.
*/
class SpectralDiffusion
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < kStages; ++s)
            {
                stages[(size_t) (c * kStages + s)].prepare ((int) (sampleRate * 0.14) + 8);
                auto& lfo = lfos[(size_t) (c * kStages + s)];
                lfo.prepare (sampleRate);
                lfo.reset (0.13f * (float) (c * kStages + s));
                lfo.setRate (0.07f + 0.043f * (float) s + (c == 1 ? 0.021f : 0.0f));
            }

        amount.prepare (sampleRate, 60.0f);
        size.prepare (sampleRate, 80.0f);
        reset();
    }

    void reset()
    {
        for (auto& s : stages) s.clear();
        // The stage LFOs are state: left where the last patch stopped them they smear the first
        // moments of the next one differently every time.
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < kStages; ++s)
                lfos[(size_t) (c * kStages + s)].reset (0.13f * (float) (c * kStages + s));
        amount.reset(); size.reset();
    }

    void setParams (float amount01, float sizeScale) noexcept
    {
        amount.setTarget (clampf (amount01, 0.0f, 1.0f));
        size.setTarget (clampf (sizeScale, 0.35f, 2.5f));
    }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float a = amount.next();
            const float scale = size.next();
            const float coefficient = 0.42f + 0.36f * a;
            const float stretch = (0.28f + 0.72f * a) * scale;

            for (int c = 0; c < 2; ++c)
            {
                float* buf = c == 0 ? l : r;
                float x = buf[i];
                const float dry = x;

                for (int s = 0; s < kStages; ++s)
                {
                    const size_t idx = (size_t) (c * kStages + s);
                    const float baseMs = kDelayMs[s] * (c == 1 ? 1.117f : 1.0f);
                    const float samples = baseMs * stretch * 0.001f * (float) sr;
                    stages[idx].set (samples, coefficient);
                    x = stages[idx].processModulated (x, lfos[idx].next() * kModSamples * 0.001f * (float) sr);
                }

                buf[i] = dry + a * (x - dry);
            }
        }
    }

private:
    static constexpr int kStages = 6;
    static constexpr float kDelayMs[kStages] { 13.7f, 21.3f, 32.9f, 47.1f, 63.7f, 89.3f };
    static constexpr float kModSamples = 0.55f;   // ms of movement per stage

    double sr = 48000.0;
    Allpass stages[2 * kStages];
    LFO lfos[2 * kStages];
    SmoothParam amount, size;
};

} // namespace am::fx
