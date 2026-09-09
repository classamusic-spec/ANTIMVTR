#pragma once

#include "WaveSource.h"
#include <memory>

namespace am
{

enum class SourceType : uint8_t { Wave = 0, Dust, Impact, Sample, Gesture, Count };

/**
    Owns every source of a voice and mixes the active ones.

    SINGLE mode renders only the selected source (the Main page workflow);
    LAYER mode renders every source at its own level.
*/
class SourceEngine
{
public:
    SourceEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void noteOn (const NoteState& note, const ParamValues& params);
    void noteOff();

    /** Renders the mixed sources (overwrite) into l/r. */
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note);

    bool isActive() const noexcept;
    float energy() const noexcept;

    SourceBase* source (SourceType t) noexcept { return sources[(size_t) t].get(); }
    const SourceBase* source (SourceType t) const noexcept { return sources[(size_t) t].get(); }

    /** Registers a source implementation for a slot (used by later phases: Dust, Impact, Sample, Gesture). */
    void setSource (SourceType t, std::unique_ptr<SourceBase> s) { sources[(size_t) t] = std::move (s); }

private:
    std::array<std::unique_ptr<SourceBase>, (size_t) SourceType::Count> sources;
    std::array<float, kMaxBlockSize> scratchL {}, scratchR {};
    double sr = 48000.0;
    int maxBlock = 0;
    bool gate = false;
    int  selected = 0;
    bool layer = false;

    static Param levelParam (SourceType t) noexcept;
};

} // namespace am
