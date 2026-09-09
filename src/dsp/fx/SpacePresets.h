#pragma once

#include "state/ParameterRegistry.h"

#include <array>

namespace am
{

/**
    SPACE TYPES — curated rack configurations.

    A Space is not a hidden algorithm: it is a *preset for the FX rack*. When
    `space.type` changes, the host side calls `SpacePresets::apply()` on the
    message thread, which writes the curated enables and settings into the
    ordinary rack parameters (space.dist.* … space.limiter.on). Everything the
    engine renders afterwards comes from those parameters plus the four
    macros, so every knob of a Space stays visible, automatable and editable
    on the advanced page.

    The only thing that is not a parameter is the module *order* (and the
    octave-up regeneration SHIMMER needs), which the engine reads from
    `routing()` at the start of every block.
*/
namespace SpacePresets
{

enum Type
{
    Nebula = 0,   ///< wide, soft, evolving cloud
    Void,         ///< huge dark cavern
    Chamber,      ///< small realistic room
    Orbit,        ///< synced delays with regeneration
    Dream,        ///< soft pitched tails
    Machine,      ///< saturation and metallic combs
    Shimmer,      ///< octave-up reverb feedback
    Dust,         ///< granular clouds
    NumTypes
};

/** Rack slots, in their default order. */
enum class Slot : uint8_t
{
    Distortion = 0,
    Chorus,
    Shifter,
    Delay,
    Granular,
    Diffusion,
    Reverb,
    EQ,
    Compressor,
    Limiter,
    Count
};

constexpr int kNumSlots = (int) Slot::Count;

/** Module order for one Space, plus the parameters that cannot be knobs. */
struct Routing
{
    std::array<uint8_t, kNumSlots> order { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
    float shimmerAmount = 0.0f;      ///< octave-up regeneration inside the reverb (0 = off)
    float shimmerSemitones = 12.0f;

    bool operator== (const Routing& other) const noexcept
    {
        return order == other.order
            && std::abs (shimmerAmount - other.shimmerAmount) < 1.0e-6f
            && std::abs (shimmerSemitones - other.shimmerSemitones) < 1.0e-6f;
    }

    bool operator!= (const Routing& other) const noexcept { return ! (*this == other); }
};

/** Number of curated Spaces (matches the space.type choice list). */
int count() noexcept;

/** Short display name, e.g. "NEBULA". */
const char* name (int type) noexcept;

/** One-line description for the UI / preset browser. */
const char* description (int type) noexcept;

/** Module order and shimmer configuration for a Space. */
Routing routing (int type) noexcept;

/**
    Writes the curated rack settings for `type` into `values`.

    Message thread only (it is a preset recall, not a DSP operation). Only the
    rack parameters are touched: space.type and the four macros (mix, size,
    tone, feedback) are left exactly as the user set them, so switching Spaces
    never steals the performance controls. Every value is clamped to its
    declared parameter range.
*/
void apply (int type, ParamValues& values);

} // namespace SpacePresets
} // namespace am
