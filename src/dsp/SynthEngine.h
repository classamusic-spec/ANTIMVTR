#pragma once

#include "VoiceManager.h"
#include "fracture/FractureEngine.h"
#include "fx/SpaceEngine.h"
#include "fx/MasterSection.h"
#include "mod/ControlGraph.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

/**
    SYNTH ENGINE — the complete audio path.

        MIDI → VoiceManager → [Source → Matter → Evolve] → voice mix
             → Fracture → Space → Master → output

    Host-agnostic: the plugin, the tests and the offline renderer all drive
    this class the same way. Diagnostics are always available but cost
    nothing when nobody reads them.
*/
class SynthEngine
{
public:
    SynthEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /**
        Renders `numSamples` into out (stereo, overwrite). MIDI events are
        applied sample-accurately (the block is split at event positions).
        `hostParams` are the base parameter values for this block.
    */
    void process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi,
                  const ParamValues& hostParams, const TransportInfo& transport);

    /** Latency introduced by Fracture / Space (host should be informed when it changes). */
    int latencySamples() const noexcept { return fracture.latencySamples() + space.latencySamples(); }

    Diagnostics& diagnostics() noexcept { return diag; }
    const Diagnostics& diagnostics() const noexcept { return diag; }
    const ControlGraph& control() const noexcept { return controlGraph; }
    ControlGraph& control() noexcept { return controlGraph; }
    VoiceManager& voiceManager() noexcept { return voices; }

    int activeVoices() const noexcept { return voices.activeVoiceCount(); }
    double sampleRate() const noexcept { return sr; }
    Quality quality() const noexcept { return currentQuality; }

private:
    void renderSegment (float* outL, float* outR, int numSamples, const RenderContext& ctx);
    void publishSnapshots (const float* outL, const float* outR, int numSamples, const RenderContext& ctx);
    void applyGlobalSettings (const ParamValues& params);

    ControlGraph  controlGraph;
    VoiceManager  voices;
    FractureEngine fracture;
    SpaceEngine   space;
    MasterSection master;
    Diagnostics   diag;

    std::array<float, kMaxBlockSize> mixL {}, mixR {}, srcL {}, srcR {};

    double  sr = 48000.0;
    int     maxBlock = 0;
    Quality currentQuality = Quality::Normal;
    int     currentMaxVoices = kDefaultVoices;
    uint64_t sampleTime = 0;
    bool    prepared = false;

    // Level accumulators for snapshots
    LevelMeter stageMeters[(int) Stage::Count];
};

} // namespace am
