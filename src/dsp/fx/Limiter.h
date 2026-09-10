#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    LIMITER — the polite one at the end of the SPACE rack.

    Zero latency by design: the master section owns the safety limiter and
    reports the plugin's latency, so SPACE must not add any.

    Two stages, because a zero-latency limiter that slams the gain down the
    instant a peak arrives puts a broadband click into every transient (it is
    visible as a vertical stripe in a spectrogram):

      1. a smoothed gain path — soft knee, ~1.2 ms attack, ~120 ms release
         with a short hold, which does almost all of the work inaudibly;
      2. a memoryless soft ceiling that catches whatever the smoothed path
         let through. It is continuous and only bends samples above the knee,
         so overshoots turn into a little low-order harmonic distortion
         instead of a click, and the output can never leave the ceiling.
*/
class Limiter
{
public:
    static constexpr float kCeiling = 0.97f;     ///< nothing ever leaves above this
    static constexpr float kThreshold = 0.86f;   ///< where the smoothed gain path starts working

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        attackCoeff  = 1.0f - (float) std::exp (-1.0 / (0.0012 * sr));
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
            // The smoothed path aims at the threshold, not at the ceiling, so once it
            // has settled the soft ceiling below has nothing left to bend: a steady
            // loud tone comes out level and clean instead of gaining a third harmonic.
            const float target = peak > kThreshold ? clampf (kThreshold / peak, 0.02f, 1.0f) : 1.0f;

            if (target < gain)
            {
                gain += (target - gain) * attackCoeff;   // smooth attack: no click on transients
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
            l[i] = softCeiling (l[i] * gain);
            r[i] = softCeiling (r[i] * gain);
        }
    }

private:
    /** Continuous soft ceiling: transparent below the knee, asymptotic at the ceiling. */
    static inline float softCeiling (float x) noexcept
    {
        const float a = std::abs (x);
        if (a <= kThreshold) return x;
        const float headroom = kCeiling - kThreshold;
        const float shaped = kThreshold + headroom * fastTanh ((a - kThreshold) / headroom);
        return x < 0.0f ? -shaped : shaped;
    }

    double sr = 48000.0;
    float gain = 1.0f, attackCoeff = 0.5f, releaseCoeff = 0.999f, reduction = 1.0f;
    int hold = 0, holdSamples = 192;
};

} // namespace am::fx
