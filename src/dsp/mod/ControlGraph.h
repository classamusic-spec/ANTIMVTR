#pragma once

#include "state/ParameterRegistry.h"
#include "core/Smoothing.h"

namespace am
{

/**
    CONTROL GRAPH

        effective = clamp (smooth (base) + modulation)

    `base` is the host-owned parameter value (automation included). Modulation
    contributions are added per block and never written back to the host.
    Phase 0 provides base smoothing and mono modulation; polyphonic
    modulation arrives with the modulation engine (Phase 10).
*/
class ControlGraph
{
public:
    ControlGraph();

    void prepare (double sampleRate, int maxBlockSize);

    /** Jumps every value to the given base immediately (no smoothing). */
    void resetTo (const ParamValues& base);

    /** Adds a monophonic modulation contribution (natural units) for the current block. */
    void addModulation (Param p, float amount) noexcept { modulation[(size_t) paramIndex (p)] += amount; }

    /** Computes the effective values for a block of `numSamples`. */
    void update (const ParamValues& base, int numSamples);

    const ParamValues& values() const noexcept { return effective; }
    const ParamValues& baseValues() const noexcept { return lastBase; }
    float value (Param p) const noexcept { return effective[(size_t) paramIndex (p)]; }

    /** Modulation contribution applied on the last update (for the UI / trace view). */
    float modulationOf (Param p) const noexcept { return lastModulation[(size_t) paramIndex (p)]; }

    /** Advances only when `update()` actually changed an effective value. Consumers that keep
        their own copy of the values (per-voice modulation) use it to skip a needless refresh. */
    uint32_t generation() const noexcept { return gen; }

private:
    ParamValues effective {}, lastBase {}, modulation {}, lastModulation {};
    uint32_t gen = 1;
    std::array<float, kNumParams> smoothed {};
    std::array<float, kNumParams> coeffPerSample {};
    std::array<float, kNumParams> blockCoeff {};   ///< coeffPerSample ^ numSamples, cached per slice length
    int    cachedBlockSize = -1;
    double sr = 48000.0;
};

} // namespace am
