#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    Two-head crossfaded delay-line pitch shifter.

    Two read heads travel through a short window at a rate derived from the
    interval; a raised-cosine crossfade (the two windows sum to unity) hides
    the wrap points. It is not a phase vocoder — it is the classic, cheap,
    slightly grainy shifter that shimmer reverbs have always used, and it is
    stable inside a feedback loop, which is exactly what SHIMMER needs.
*/
class PitchShifter
{
public:
    void prepare (double sampleRate, float windowMs = 80.0f)
    {
        window = std::max (256.0f, windowMs * 0.001f * (float) sampleRate);
        line.prepare ((int) (window * 2.5f) + 64);
        reset();
    }

    void reset()
    {
        line.clear();
        phase = 0.0f;
    }

    /** Positive = up. Recomputed rarely, so std::pow is fine here. */
    void setSemitones (float semitones) noexcept
    {
        rate = std::pow (2.0f, clampf (semitones, -24.0f, 24.0f) / 12.0f);
    }

    inline float process (float x) noexcept
    {
        line.write (x);

        phase += (1.0f - rate) / window;
        phase -= std::floor (phase);

        const float phaseB = phase >= 0.5f ? phase - 0.5f : phase + 0.5f;
        const float d1 = 32.0f + phase  * window;
        const float d2 = 32.0f + phaseB * window;
        const float w1 = 0.5f - 0.5f * fastSin01 (phase + 0.25f);
        const float w2 = 0.5f - 0.5f * fastSin01 (phaseB + 0.25f);

        return line.readHermite (d1) * w1 + line.readHermite (d2) * w2;
    }

private:
    DelayLine line;
    float window = 4096.0f, phase = 0.0f, rate = 2.0f;
};

} // namespace am::fx
