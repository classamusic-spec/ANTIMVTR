#include "EvolveEngine.h"

namespace am
{

void EvolveEngine::prepare (double sampleRate, int) { sr = sampleRate; reset(); }
void EvolveEngine::reset() { motionPhase = 0.0; }
void EvolveEngine::noteOn (const NoteState&, const ParamValues&) {}

void EvolveEngine::apply (MatterEngine& matter, const RenderContext& ctx, const NoteState&)
{
    // Phase 0: no transformation. Keep the motion clock running so later
    // operators start from a consistent state.
    const double speed = 0.05 + 8.0 * (double) ctx.param (Param::evolveSpeed);
    motionPhase += speed * (double) ctx.numSamples / ctx.sampleRate;
    motionPhase -= std::floor (motionPhase);
    juce::ignoreUnused (matter);
}

} // namespace am
