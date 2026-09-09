#pragma once

#include "SourceBase.h"

namespace am
{

/**
    WAVE source.

    Phase 0/2 implementation: a clean sine oscillator with level, octave /
    semitone / fine tuning, phase reset and free-running phase. The full
    wavetable engine (procedural tables, position/scan/morph, unison, FM/PM/
    AM/ring/sync) replaces this implementation behind the same interface.
*/
class WaveSource final : public SourceBase
{
public:
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void noteOn (const NoteState& note, const ParamValues& params) override;
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) override;
    float energy() const noexcept override { return lastLevel; }

private:
    double sr = 48000.0;
    double phase = 0.0;
    float  lastLevel = 0.0f;
    uint32_t phaseSeed = 1;
};

} // namespace am
