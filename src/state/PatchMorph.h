#pragma once

#include "StateManager.h"

namespace am
{

/**
    A/B MORPH — safe interpolation between two patches (SPEC §44–46).

    Continuous parameters interpolate in the host's normalised curve (so a
    frequency or time morphs perceptually), integers round, choices and
    booleans snap at the half-way point, and every value is clamped to its
    registry range. The JSON sections (fracture, mod, space, sample, …) and
    the metadata come from the nearer patch; engines that can blend their
    own tables (the Fracture table) do so in the plugin layer.
*/
struct PatchMorph
{
    /** t = 0 → a, t = 1 → b. */
    static ParamValues interpolate (const ParamValues& a, const ParamValues& b, float t) noexcept;

    /** Parameters as above; sections and metadata from the nearer patch (t < 0.5 → a). */
    static PatchState interpolate (const PatchState& a, const PatchState& b, float t);
};

} // namespace am
