#pragma once

#include "dsp/RenderContext.h"

namespace am
{

/**
    Base class for every energy source (WAVE, DUST, IMPACT, SAMPLE, GESTURE).

    Sources render a stereo pair of excitation signals. Matter derives its
    excitation from that pair; the direct path can also be heard when
    shape.mix is below 1.

    Real-time rules apply: no allocation after prepare(), no locks.
*/
class SourceBase
{
public:
    virtual ~SourceBase() = default;

    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    virtual void reset() = 0;

    /** Called when the voice starts (or retriggers). */
    virtual void noteOn (const NoteState& note, const ParamValues& params) = 0;

    /** Called when the key is released. Sources may keep sounding (release tails). */
    virtual void noteOff() {}

    /** Renders `n` samples into l/r (overwrite, not accumulate). */
    virtual void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) = 0;

    /** True while the source still produces energy. Continuous sources return true while gated. */
    virtual bool isActive() const noexcept { return true; }

    /** Approximate output energy (0..1) for diagnostics. */
    virtual float energy() const noexcept { return 0.0f; }
};

} // namespace am
