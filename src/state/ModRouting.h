#pragma once

#include "ParameterRegistry.h"

#include <juce_data_structures/juce_data_structures.h>

namespace am
{

//==============================================================================
/**
    Every modulation source ANTI-MATR can route.

    The enum values are part of the preset format only through
    `modSourceId()` — the numeric values may be reordered, the *identifiers*
    are permanent.
*/
enum class ModSource : uint8_t
{
    None = 0,
    LFO1, LFO2, LFO3, LFO4,
    Env1, Env2, Env3, Env4,
    Chaos1, Chaos2, Chaos3, Chaos4,
    Macro1, Macro2, Macro3, Macro4, Macro5, Macro6, Macro7, Macro8,
    Velocity, KeyTrack, Pressure, ModWheel, PitchBend, Timbre, NoteRandom, Gate,
    Count
};

constexpr int kNumModSources = static_cast<int> (ModSource::Count);

/** Coarse grouping used by the UI menus. */
enum class ModSourceGroup : uint8_t { LFO = 0, Envelope, Chaos, Macro, Note, Count };

/** Permanent identifier used in JSON ("lfo1", "velocity", …). Never change these. */
const char* modSourceId (ModSource s) noexcept;

/** Customer-facing name ("LFO 1", "Velocity"). */
const char* modSourceName (ModSource s) noexcept;

/** Parses a permanent identifier; returns `ModSource::None` when unknown. */
ModSource modSourceFromId (juce::StringRef id) noexcept;

ModSourceGroup modSourceGroup (ModSource s) noexcept;
const char* modSourceGroupName (ModSourceGroup g) noexcept;

/** Natural polarity: true for sources that swing -1 … 1, false for 0 … 1 sources. */
bool modSourceIsBipolar (ModSource s) noexcept;

/** True for sources that exist once per voice (envelopes, note sources, retriggered LFOs). */
bool modSourceIsPerVoice (ModSource s, const ParamValues& params) noexcept;

/** The `mod.lfoN.*` / `mod.envN.*` / `mod.chaosN.*` / `macro.N` slot index (0-based), or -1. */
int modSourceSlot (ModSource s) noexcept;

//==============================================================================
/**
    One modulation assignment: a source, a destination parameter and how much
    of the destination's range the source may move.

    `depth` is a signed fraction of the target's full range in natural units:
    depth = 0.5 on `shape.decay` (0 … 1) moves it by ±0.5 for a bipolar source
    and 0 … +0.5 for a unipolar one. `curve` bends the source (-1 … 1, 0 =
    linear) and `bipolar` chooses how the source is presented, independently
    of its natural polarity.

    Plain data: the whole table is trivially copyable and travels to the audio
    thread through `RealtimeHandoff`.
*/
struct ModRouting
{
    ModSource source  = ModSource::None;
    Param     target  = Param::Count;   ///< Param::Count = unassigned
    float     depth   = 0.0f;           ///< -1 … 1 of the target's range
    float     curve   = 0.0f;           ///< -1 … 1 (0 = linear)
    bool      bipolar = true;           ///< present the source as -1…1 (else 0…1)
    bool      enabled = true;

    bool operator== (const ModRouting& o) const noexcept
    {
        return source == o.source && target == o.target && depth == o.depth
            && curve == o.curve && bipolar == o.bipolar && enabled == o.enabled;
    }
    bool operator!= (const ModRouting& o) const noexcept { return ! (*this == o); }
};

//==============================================================================
/**
    The complete modulation matrix: up to 64 routings kept in a deterministic
    order (by source, then by target index) so two tables with the same
    content always serialise, compile and sum identically.

    Lives in `PatchState::mod` as JSON and reaches the audio thread through
    `ModulationEngine::publishRoutings()`. `toVar` / `fromVar` are message
    thread only (they touch `juce::var`); everything else is allocation free.
*/
class ModRoutingTable
{
public:
    static constexpr int kMaxRoutings = 64;

    //==========================================================================
    int  size() const noexcept                        { return count; }
    bool isEmpty() const noexcept                     { return count == 0; }
    bool isFull() const noexcept                      { return count >= kMaxRoutings; }
    const ModRouting& operator[] (int i) const noexcept { return routings[(size_t) juce::jlimit (0, kMaxRoutings - 1, i)]; }
    const ModRouting* begin() const noexcept          { return routings.data(); }
    const ModRouting* end() const noexcept            { return routings.data() + count; }

    /** Adds a routing (sanitised). Returns its index, or -1 when invalid, duplicate or full. */
    int  add (const ModRouting& r) noexcept;
    bool remove (int index) noexcept;
    void clear() noexcept { count = 0; routings.fill (ModRouting{}); }

    bool setDepth   (int index, float depth) noexcept;
    bool setCurve   (int index, float curve) noexcept;
    bool setBipolar (int index, bool b) noexcept;
    bool setEnabled (int index, bool b) noexcept;

    /** Index of the routing connecting `s` to `target`, or -1. */
    int  indexOf (ModSource s, Param target) const noexcept;
    /** Number of enabled routings pointing at `target`. */
    int  countForTarget (Param target) const noexcept;
    /** Number of enabled routings. */
    int  numEnabled() const noexcept;

    //==========================================================================
    /** A routing is valid when the source exists and the target is a modulatable parameter. */
    static bool isValid (const ModRouting& r) noexcept;
    /** Clamps depth / curve into range (does not make an invalid routing valid). */
    static ModRouting sanitised (ModRouting r) noexcept;

    //==========================================================================
    /** JSON writer; round-trips through `fromVar`. */
    juce::var toVar() const;
    /** Tolerant JSON reader — invalid or duplicate routings are dropped (and reported). */
    static ModRoutingTable fromVar (const juce::var& v, juce::String* warnings = nullptr);

    bool operator== (const ModRoutingTable& o) const noexcept;
    bool operator!= (const ModRoutingTable& o) const noexcept { return ! (*this == o); }

private:
    void sort() noexcept;

    int count = 0;
    std::array<ModRouting, kMaxRoutings> routings {};
};

static_assert (std::is_trivially_copyable_v<ModRoutingTable>, "ModRoutingTable must be safe to hand to the audio thread");

} // namespace am
