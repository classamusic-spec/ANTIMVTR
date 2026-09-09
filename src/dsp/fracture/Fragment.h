#pragma once

#include "core/Types.h"

#include <array>

namespace am
{

/** FRACTURE processing modes (mirrors the `fracture.mode` choice parameter). */
enum class FractureMode : uint8_t { Spectral = 0, Rhythmic, Transient, Evolve, Count };

/** Sequencer playback directions (mirrors `fracture.direction`). */
enum class FractureDirection : uint8_t { Forward = 0, Backward, PingPong, Random, Count };

//==============================================================================
/**
    One spectral fragment. The spectrum is split into 8 / 16 / 32 perceptually
    spaced bands; each band carries this settings block. All fields are plain
    floats so the whole table is trivially copyable and can be handed to the
    audio thread through `RealtimeHandoff`.
*/
struct Fragment
{
    float pitch       = 0.0f;   ///< semitones, -24 … +24 (added to `fracture.pitch`)
    float delay       = 0.0f;   ///< normalised 0 … 1 (fraction of the 1 s maximum, scaled by `fracture.delay`)
    float pan         = 0.0f;   ///< -1 = left, +1 = right
    float decay       = 0.5f;   ///< 0 … 1, scales the feedback ring time (with `fracture.decay`)
    float probability = 1.0f;   ///< 0 … 1, chance the fragment is active on a step
    float feedback    = 0.0f;   ///< 0 … 1, scaled by `fracture.feedback`
    float spread      = 1.0f;   ///< 0 … 1, how strongly `fracture.spread` scatters this fragment
    float gain        = 1.0f;   ///< 0 … 2 linear
};

//==============================================================================
/**
    One fragment-sequencer step. `mask` selects which fragments are open on this
    step (bit f = fragment f). `evolve` and `shape` are routed by the principal
    (Evolve amount / Shape macro) and are carried through untouched by the engine.
*/
struct SequencerStep
{
    uint32_t mask        = 0xFFFFFFFFu; ///< fragment selection bitmask
    float    gate        = 1.0f;        ///< 0 … 1 step level
    float    pitch       = 0.0f;        ///< semitones added to every open fragment
    float    pan         = 0.0f;        ///< -1 … 1 added to every open fragment
    float    gain        = 1.0f;        ///< 0 … 2 linear
    float    probability = 1.0f;        ///< 0 … 1, multiplied with `fracture.probability`
    float    evolve      = 0.0f;        ///< 0 … 1 Evolve amount for the principal to route
    float    shape       = 0.0f;        ///< 0 … 1 Shape macro for the principal to route
};

//==============================================================================
/**
    The complete free-form Fracture state: per-fragment settings plus the
    sequencer pattern. Lives in `PatchState::fracture` as JSON and travels to
    the audio thread via `FractureEngine::publishTable()`.

    POD-ish and trivially copyable: never allocates, safe to hold on the audio
    thread. `fromVar` / `toVar` are message-thread only (they touch `juce::var`).
*/
struct FractureTable
{
    int numFragments = 16;  ///< informational; the engine follows `fracture.fragments`
    int numSteps     = 8;   ///< informational; the engine follows `fracture.steps`

    std::array<Fragment, kMaxFractureFragments> fragments {};
    std::array<SequencerStep, kMaxSequencerSteps> steps {};

    /** A musical starting point: rising delays, alternating octaves, spread pans. */
    static FractureTable makeDefault();

    /** Tolerant JSON reader — missing fields default, out-of-range values clamp. */
    static FractureTable fromVar (const juce::var& v);

    /** JSON writer, round-trips through `fromVar`. */
    juce::var toVar() const;
};

} // namespace am
