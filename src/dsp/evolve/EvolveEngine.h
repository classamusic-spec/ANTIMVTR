#pragma once

#include "dsp/matter/MatterEngine.h"

namespace am
{

/**
    EVOLVE ENGINE — transformations applied directly to Matter nodes
    (BEND, MELT, TEAR, MAGNET, GRAVITY, SCATTER, FREEZE, CRUSH).

    Runs once per block per voice before Matter renders. Phase 0: identity.
*/
class EvolveEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void noteOn (const NoteState& note, const ParamValues& params);

    /** Applies the operators to the Matter graph for the coming block. */
    void apply (MatterEngine& matter, const RenderContext& ctx, const NoteState& note);

private:
    double sr = 48000.0;
    double motionPhase = 0.0;
};

} // namespace am
