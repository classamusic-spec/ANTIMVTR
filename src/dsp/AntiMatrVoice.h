#pragma once

#include "Envelope.h"
#include "source/SourceEngine.h"
#include "matter/MatterEngine.h"
#include "evolve/EvolveEngine.h"
#include "core/Smoothing.h"

namespace am
{

/**
    One polyphonic voice: Source → Matter (+Evolve) → amplitude envelope.
*/
class AntiMatrVoice
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    struct StartInfo
    {
        int   midiNote = 60;
        float velocity = 1.0f;
        int   channel  = 1;
        uint32_t noteId = 0;
        bool  legato   = false;     ///< true: keep envelopes running, only change pitch
        double glideSeconds = 0.0;
        double fromFrequency = 0.0; ///< glide start (0 = none)
        float pitchBendSemis = 0.0f;
        float pressure = 0.0f;
        float modWheel = 0.0f;
        Quality quality = Quality::Normal;
    };

    void start (const StartInfo& info, const ParamValues& params);
    void release();
    void kill();                     ///< fast fade for stealing

    void setPitchBend (float semis) noexcept { note.pitchBendSemis = semis; }
    void setPressure (float p) noexcept      { note.pressure = p; }
    void setModWheel (float w) noexcept      { note.modWheel = w; }
    void setSustained (bool s) noexcept      { note.sustained = s; }

    /**
        Renders and ACCUMULATES into outL/outR. The raw source is accumulated
        into srcL/srcR when those are non-null (diagnostics tap).
    */
    void render (float* outL, float* outR, float* srcL, float* srcR, int n, const RenderContext& ctx);

    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return active && ! note.gate; }
    bool isGated() const noexcept { return note.gate; }
    int  midiNote() const noexcept { return note.midiNote; }
    int  channel() const noexcept { return note.channel; }
    uint32_t noteId() const noexcept { return note.noteId; }
    float envelopeLevel() const noexcept { return ampEnv.getLevel(); }
    float energy() const noexcept { return lastEnergy; }
    const NoteState& noteState() const noexcept { return note; }

    const MatterEngine& matter() const noexcept { return matterEngine; }
    MatterEngine& matter() noexcept { return matterEngine; }

private:
    void updateFrequency (const RenderContext& ctx);

    SourceEngine sourceEngine;
    MatterEngine matterEngine;
    EvolveEngine evolveEngine;
    ADSREnvelope ampEnv;

    NoteState note;
    bool active = false;
    float velocityGain = 1.0f;
    float lastEnergy = 0.0f;
    double sr = 48000.0;

    // pitch handling
    double glideLog = 0.0;          // current log2 frequency during glide
    double glideTargetLog = 0.0;
    double glideCoeff = 0.0;        // per-sample coefficient (0 = no glide)
    bool gliding = false;

    std::array<float, kMaxBlockSize> bufL {}, bufR {}, matL {}, matR {};
};

} // namespace am
