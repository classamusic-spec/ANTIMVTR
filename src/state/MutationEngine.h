#pragma once

#include "ParameterRegistry.h"
#include "core/Random.h"

namespace am
{

enum class MutationStrength : uint8_t { Subtle = 0, Evolve, Extreme };

/**
    DNA-aware mutation. Never "randomize all": every parameter carries a
    mutation category, and the engine applies bounded, category-weighted
    changes so the result stays a playable relative of the original.

    Phase 0 implementation covers the mechanics (categories, weights, safe
    ranges, deterministic seeds). Later phases refine preferred regions and
    distributions per parameter.
*/
class MutationEngine
{
public:
    using CategoryMask = uint32_t;
    static constexpr CategoryMask kAllCategories = 0xFFFFFFFFu;

    static constexpr CategoryMask maskFor (MutationCategory c) noexcept { return 1u << (uint32_t) c; }

    /** Mutates `values` in place. Deterministic for a given seed. */
    static void mutate (ParamValues& values, MutationStrength strength, uint32_t seed,
                        CategoryMask categories = kAllCategories);

    /** Produces a fresh random-but-safe patch starting from defaults. */
    static void randomize (ParamValues& values, uint32_t seed);

    /** Safe mutation range for a parameter (subset of its full range). */
    static void safeRange (Param p, float& lo, float& hi) noexcept;

    /** Relative weight 0..1 of how much a parameter should move. */
    static float weight (Param p) noexcept;
};

} // namespace am
