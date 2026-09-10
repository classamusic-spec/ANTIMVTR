#pragma once

#include "dsp/RenderContext.h"
#include "core/RealtimeUtils.h"
#include "core/Smoothing.h"

namespace am
{

class SafetyMonitor;

/**
    Final output stage: gain, DC blocking, non-finite scrubbing and a fast
    safety limiter. Always active — this is what keeps speakers safe when
    an experimental material misbehaves.
*/
class MasterSection
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Processes in place. Reports events to the safety monitor if provided. */
    void process (float* l, float* r, int n, const RenderContext& ctx, SafetyMonitor* safety);

    float gainReduction() const noexcept { return currentReduction; }

private:
    double sr = 48000.0;
    OnePoleSmoother gainSmoother;
    DCBlocker dcL, dcR;
    float envelope = 0.0f;         // limiter detector
    float releaseCoeff = 0.0f;
    float currentReduction = 1.0f;
    float dcEstimate = 0.0f;       // slow (~300 ms) running mean for DC detection
    static constexpr float kCeiling = 0.98f;
    static constexpr float kHardCeiling = 4.0f;
};

} // namespace am
