#pragma once

#include "ControlGraph.h"
#include "ModSources.h"
#include "ModulationSnapshot.h"
#include "core/RealtimeHandoff.h"

namespace am
{

//==============================================================================
/**
    One routing resolved into exactly what the audio thread needs: no lookups,
    no branches on descriptors, everything pre-scaled into natural units.
*/
struct CompiledRouting
{
    uint16_t target     = 0;      ///< parameter index
    uint8_t  source     = 0;      ///< ModSource
    uint8_t  targetSlot = 0;      ///< index into ModPlan::polyTargets (poly routings only)
    bool     wantBipolar   = true;
    bool     sourceBipolar = true;
    float    scale = 0.0f;        ///< depth × (max - min): natural units per unit of source
    float    curve = 0.0f;
};

//==============================================================================
/**
    The routing table compiled for the current block.

    Mono routings are summed by `ModulationEngine` into `ControlGraph`; poly
    routings are applied by every voice to its own copy of the effective
    parameters. `polyTargets` lists each destination once so a voice only
    touches the entries it actually changes.
*/
struct ModPlan
{
    static constexpr int kMax = ModRoutingTable::kMaxRoutings;

    int numMono = 0, numPoly = 0, numPolyTargets = 0;
    uint32_t polySources = 0;     ///< bit (int) ModSource set for every per-voice source in use

    std::array<CompiledRouting, kMax> mono {};
    std::array<CompiledRouting, kMax> poly {};
    std::array<uint16_t, kMax> polyTargets {};

    bool isEmpty() const noexcept { return numMono == 0 && numPoly == 0; }
    bool usesSource (ModSource s) const noexcept { return (polySources & (1u << (uint32_t) s)) != 0; }
};

//==============================================================================
/**
    The per-voice half of the modulation system.

    Owns this voice's retriggered LFOs, its four modulation envelopes and the
    note-derived sources, and turns the engine's *global* effective parameters
    into the values this particular voice hears. Two voices with different
    velocities therefore render different Shape / Source / Evolve settings.

    Allocation free: the parameter copy lives inside the object and only the
    routed destinations are recomputed each control slice.
*/
class VoiceModulator
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    /** Note-on: retriggers the per-voice LFOs and starts the envelopes. */
    void noteOn (const NoteState& note, uint32_t seed) noexcept;
    /** Note-off: releases the envelopes and drops the gate source. */
    void noteOff() noexcept;

    /**
        Advances the per-voice sources by one control slice and returns the
        parameter values this voice must render with — `&global` when nothing
        is routed per voice, otherwise the voice's own clamped copy.
    */
    const ParamValues* process (const ParamValues& global, const ModPlan* plan, int numSamples,
                                double sampleRate, const NoteState& note, const TransportInfo& transport,
                                uint32_t paramGeneration) noexcept;

    /** Value of a source for this voice, natural polarity (0 for global sources). */
    float value (ModSource s) const noexcept;

    /** The values this voice last rendered with. Only meaningful when per-voice routings exist. */
    const ParamValues& parameters() const noexcept { return voiceParams; }

    /** Contribution this voice applied to `plan->polyTargets[slot]` (natural units). */
    float deltaAt (int slot) const noexcept { return slot >= 0 && slot < numDeltas ? deltas[(size_t) slot] : 0.0f; }
    int   numContributions() const noexcept { return numDeltas; }

    float envelopeLevel (int index) const noexcept { return envelopes[(size_t) juce::jlimit (0, kNumEnvelopes - 1, index)].value(); }

private:
    std::array<ModLFO, kNumLFOs> lfos;
    std::array<ModEnvelope, kNumEnvelopes> envelopes;
    NoteSourceValues notes;
    ParamValues voiceParams {};
    std::array<float, ModPlan::kMax> deltas {};
    int numDeltas = 0;
    double sr = 48000.0;

    // Copying the whole parameter array for every voice on every block is the single most
    // expensive thing per-voice modulation can do (64 voices x kNumParams floats, which also
    // evicts the resonator state of the voices that follow). The copy is only needed when the
    // global values or the routing plan actually changed; otherwise the modulated slots — and
    // only those — are rewritten in place.
    const ModPlan* copiedPlan = nullptr;
    uint32_t copiedGeneration = 0;
    bool     copyValid = false;
};

//==============================================================================
/**
    MODULATION ENGINE — the global half of the system.

    Owned by `SynthEngine`. Acquires the routing table through
    `RealtimeHandoff` (message thread publishes, audio thread swaps), advances
    the four LFOs, four chaos generators and eight macros once per control
    slice, sums every *mono* routing into `ControlGraph::addModulation()`
    before `update()`, and hands voices the compiled per-voice plan.

    Nothing here allocates, locks or logs after `prepare()`.
*/
class ModulationEngine
{
public:
    /**
        Samples between control updates while modulation is active, by quality tier.

        Modulation is applied once per control slice, so the slice length sets how
        finely an LFO can move a parameter: coarser slices step audibly on pitch,
        finer slices cost one more pass over every voice. NORMAL runs at 375 Hz
        (2.7 ms at 48 kHz), HIGH and ULTRA at 750 Hz, ECO at 187 Hz.
    */
    static constexpr int controlBlockForQuality (Quality q) noexcept
    {
        switch (q)
        {
            case Quality::Eco:   return 256;
            case Quality::High:  return 64;
            case Quality::Ultra: return 64;
            default:             return 128;
        }
    }

    ModulationEngine();

    //==========================================================================
    // Audio thread
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Control-rate tier. Set from `master.quality` once per block. */
    void setQuality (Quality q) noexcept { quality = q; }

    /** Picks up a newly published routing table. Call once per host block. */
    void beginBlock (const ParamValues& params) noexcept;

    /** Restarts the global LFO fade-ins (first note of a phrase). Any thread. */
    void noteStarted() noexcept { retriggerRequest.store (true, std::memory_order_release); }

    /** Advances the global sources and adds every mono contribution to `graph`. */
    void process (ControlGraph& graph, int numSamples, const TransportInfo& transport) noexcept;

    /** How many samples the engine wants per control slice (the whole block when idle). */
    int controlBlockSize (int blockSize) const noexcept
    {
        return plan.isEmpty() ? blockSize : std::min (blockSize, controlBlockForQuality (quality));
    }

    const ModPlan& modPlan() const noexcept { return plan; }
    bool  isActive() const noexcept { return ! plan.isEmpty(); }

    /** Fills the UI snapshot. `focus` may be null when no voice is playing. */
    void fillSnapshot (ModulationSnapshot& s, const VoiceModulator* focus, int focusVoice, uint64_t sampleTime) const noexcept;

    /** Value of a global source, natural polarity. */
    float value (ModSource s) const noexcept;

    //==========================================================================
    // Message thread
    void publishRoutings (std::unique_ptr<ModRoutingTable> table) { handoff.publish (std::move (table)); }
    void messageThreadMaintenance() { handoff.collectGarbage(); }

private:
    void compile (const ParamValues& params) noexcept;

    RealtimeHandoff<ModRoutingTable> handoff;
    ModRoutingTable routings;              ///< audio-thread copy of the live table
    ModPlan plan;

    std::array<ModLFO, kNumLFOs> lfos;
    std::array<ChaosGenerator, kNumChaos> chaos;
    std::array<float, kNumMacros> macros {};

    std::array<float, kNumParams> monoMod {}, modMin {}, modMax {};
    std::array<uint8_t, kNumParams> targeted {};

    Quality quality = Quality::Normal;
    uint8_t retrigMask = 0xFF;             ///< lfoN.retrig bits the plan was compiled for
    bool    haveTable = false;
    double  sr = 48000.0;
    float   envelopeDecay = 0.02f;         ///< how fast the displayed min/max shrink back
    std::atomic<bool> retriggerRequest { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationEngine)
};

} // namespace am
