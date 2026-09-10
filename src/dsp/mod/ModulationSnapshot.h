#pragma once

#include "state/ModRouting.h"

namespace am
{

/**
    Everything the UI needs to draw modulation, published once per block
    through a `TripleBuffer` in `Diagnostics`.

    `modulation` is the total contribution applied to a parameter *right now*
    in natural units (mono routings plus the per-voice contribution of the
    most recent voice); `modMin` / `modMax` are the extremes seen over the
    last couple of seconds, which is what the knobs draw as the modulation
    range. Trivially copyable — no pointers, no juce types.
*/
struct ModulationSnapshot
{
    int      numRoutings       = 0;   ///< routings in the table
    int      numEnabled        = 0;
    int      numPolyRoutings   = 0;   ///< of those, per-voice ones
    int      controlBlock      = 0;   ///< samples between control updates
    int      focusVoice        = -1;  ///< voice the poly contributions came from
    uint64_t sampleTime        = 0;

    /** Current value of every source, in its natural polarity (-1…1 or 0…1).
        Per-voice sources report the focus voice. */
    float sourceValue[kNumModSources] {};

    /** Per parameter, in natural units. */
    float   modulation[kNumParams] {};
    float   modMin[kNumParams] {};
    float   modMax[kNumParams] {};
    uint8_t targeted[kNumParams] {};   ///< enabled routings pointing at this parameter

    float valueOf (ModSource s) const noexcept
    {
        const int i = (int) s;
        return i > 0 && i < kNumModSources ? sourceValue[i] : 0.0f;
    }

    bool isModulated (Param p) const noexcept
    {
        const int i = paramIndex (p);
        return i >= 0 && i < kNumParams && targeted[i] > 0;
    }
};

static_assert (std::is_trivially_copyable_v<ModulationSnapshot>, "ModulationSnapshot must be trivially copyable");

} // namespace am
