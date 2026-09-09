#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    LIMITER — the polite one at the end of the SPACE rack.

    Zero latency by design: the master section owns the safety limiter and
    reports the plugin's latency, so SPACE must not add any. Gain reduction is
    applied the instant a peak crosses the ceiling (no overshoot) and released
    smoothly over ~120 ms, with a soft knee so it grabs long tails and feedback
    build-ups without sounding like a brick wall.
*/
class Limiter
{
public:
    static constexpr float kCeiling = 0.94f;
    static constexpr float kKneeRatio = 0.85f;
    static constexpr float kKnee = kCeiling * kKneeRatio;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        releaseCoeff = (float) std::exp (-1.0 / (0.120 * sr));
        holdSamples = (int) (0.004 * sr);
        reset();
    }

    void reset()
    {
        gain = 1.0f;
        hold = 0;
        reduction = 1.0f;
    }

    float gainReduction() const noexcept { return reduction; }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float peak = std::max (std::abs (l[i]), std::abs (r[i]));
            float target = 1.0f;
            if (peak > kKnee)
            {
                // Soft knee from -1.4 dB, asymptotically approaching the ceiling.
                const float over = peak / kKnee;
                const float limited = kKnee * (1.0f + (1.0f / kKneeRatio - 1.0f) * fastTanh (over - 1.0f));
                target = clampf (limited / peak, 0.02f, 1.0f);
            }

            if (target < gain)
            {
                gain = target;                    // instant attack: no overshoot without lookahead
                hold = holdSamples;
            }
            else if (hold > 0)
            {
                --hold;
            }
            else
            {
                gain = target + releaseCoeff * (gain - target);
            }

            gain = sanitise (gain);
            reduction = gain;
            l[i] *= gain;
            r[i] *= gain;
        }
    }

private:
    double sr = 48000.0;
    float gain = 1.0f, releaseCoeff = 0.999f, reduction = 1.0f;
    int hold = 0, holdSamples = 192;
};

} // namespace am::fx
