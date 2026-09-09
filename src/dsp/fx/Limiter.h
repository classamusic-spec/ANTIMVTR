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
            if (peak > kCeiling * 0.75f)
            {
                // Soft knee from 75% of the ceiling, hard from the ceiling up.
                const float over = peak / (kCeiling * 0.75f);
                target = over <= 1.0f ? 1.0f : (kCeiling * 0.75f * (1.0f + 0.333f * fastTanh (over - 1.0f))) / peak;
                target = clampf (target, 0.02f, 1.0f);
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
