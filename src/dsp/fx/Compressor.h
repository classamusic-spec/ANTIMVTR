#pragma once

#include "FXCommon.h"

namespace am::fx
{

/**
    COMPRESSOR — one AMOUNT macro drives the whole thing.

        amount 0 → -4 dB threshold, 1.4:1, slow (30 ms / 260 ms): barely there
        amount 1 → -30 dB threshold, 8:1, fast (4 ms / 90 ms): pumping glue

    Stereo linked (the louder channel decides), soft knee, and makeup gain
    derived from the threshold so raising the amount does not just get louder.
*/
class Compressor
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;
        makeup.prepare (sampleRate, 40.0f);
        makeup.reset (1.0f);
        reset();
    }

    void reset()
    {
        envelope = 0.0f;
        gainState = 1.0f;
        reduction = 1.0f;
    }

    void setParams (float amount01) noexcept
    {
        const float a = clampf (amount01, 0.0f, 1.0f);
        thresholdDb = -4.0f - 26.0f * a;
        ratio = 1.4f + 6.6f * a;
        knee = 6.0f;
        const float attackMs = 30.0f - 26.0f * a;
        const float releaseMs = 260.0f - 170.0f * a;
        attackCoeff = (float) std::exp (-1.0 / (0.001 * (double) attackMs * sr));
        releaseCoeff = (float) std::exp (-1.0 / (0.001 * (double) releaseMs * sr));
        smoothCoeff = 1.0f - (float) std::exp (-1.0 / (0.0015 * sr));   // ~1.5 ms de-chatter
        // Half of the theoretical gain lost at the threshold, so it stays polite.
        makeup.setTarget (dbToGain (-thresholdDb * (1.0f - 1.0f / ratio) * 0.45f * a));
    }

    float gainReduction() const noexcept { return reduction; }

    void process (float* l, float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float peak = std::max (std::abs (l[i]), std::abs (r[i]));
            const float coeff = peak > envelope ? attackCoeff : releaseCoeff;
            envelope = peak + coeff * (envelope - peak);
            envelope = sanitise (envelope);

            const float levelDb = gainToDb (std::max (envelope, 1.0e-7f));
            const float over = levelDb - thresholdDb;
            float gainDb = 0.0f;
            if (over > knee * 0.5f)
                gainDb = -(over - over / ratio);
            else if (over > -knee * 0.5f)
            {
                const float t = over + knee * 0.5f;
                gainDb = -((1.0f - 1.0f / ratio) * t * t / (2.0f * knee));
            }

            const float target = dbToGain (gainDb);
            gainState += (target - gainState) * smoothCoeff;   // smooth the last few dB of chatter
            reduction = gainState;

            const float mk = makeup.next();
            l[i] *= gainState * mk;
            r[i] *= gainState * mk;
        }
    }

private:
    double sr = 48000.0;
    float thresholdDb = -12.0f, ratio = 2.0f, knee = 6.0f;
    float attackCoeff = 0.9f, releaseCoeff = 0.999f, smoothCoeff = 0.25f;
    float envelope = 0.0f, gainState = 1.0f, reduction = 1.0f;
    SmoothParam makeup;
};

} // namespace am::fx
