#pragma once

#include "Types.h"

namespace am
{

/** One-pole exponential smoother (control-rate or audio-rate). */
class OnePoleSmoother
{
public:
    void prepare (double sampleRate, float timeMs) noexcept
    {
        sr = sampleRate;
        setTime (timeMs);
    }

    void setTime (float timeMs) noexcept
    {
        const double samples = std::max (1.0, sr * (double) timeMs * 0.001);
        coeff = (float) std::exp (-1.0 / samples);
    }

    void reset (float value) noexcept { current = target = value; }
    void setTarget (float t) noexcept { target = t; }
    float getTarget() const noexcept  { return target; }
    float getCurrent() const noexcept { return current; }

    inline float next() noexcept
    {
        current = target + coeff * (current - target);
        return current;
    }

    /** Advance by n samples in one step (exact closed form). */
    inline float skip (int n) noexcept
    {
        const float k = std::pow (coeff, (float) n);
        current = target + k * (current - target);
        return current;
    }

    bool isSmoothing() const noexcept { return std::abs (current - target) > 1.0e-6f; }

private:
    double sr = 48000.0;
    float coeff = 0.99f, current = 0.0f, target = 0.0f;
};

/** Linear per-block ramp used for gain staging. */
class LinearRamp
{
public:
    void reset (float v) noexcept { current = target = v; step = 0.0f; remaining = 0; }

    void setTarget (float t, int numSamples) noexcept
    {
        target = t;
        remaining = numSamples > 0 ? numSamples : 1;
        step = (target - current) / (float) remaining;
    }

    inline float next() noexcept
    {
        if (remaining > 0)
        {
            current += step;
            if (--remaining == 0) current = target;
        }
        return current;
    }

    float getCurrent() const noexcept { return current; }

private:
    float current = 0.0f, target = 0.0f, step = 0.0f;
    int remaining = 0;
};

} // namespace am
