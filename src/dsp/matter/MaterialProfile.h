#pragma once

#include "core/Types.h"
#include "core/Random.h"

namespace am
{

/** Order matches the shape.materialA/B choice list. */
enum class MaterialType : uint8_t { Crystal = 0, Metal, Organic, Liquid, Membrane, String, Wood, Void, Custom, Count };

/** The seven structural anchors the FORM control morphs across (in order). */
enum class FormAnchor : uint8_t { Harmonic = 0, Stretched, Membrane, Metallic, Inharmonic, Crystalline, Stochastic, Count };

enum class StereoPattern : uint8_t { Random = 0, Alternate, Narrow };

/**
    A modal structure: log2 frequency ratios (relative to the fundamental)
    and relative amplitude weights for kMaxMatterNodes nodes, in order of
    importance (index 0 is always the fundamental, ratio 1).
*/
struct StructureTable
{
    std::array<float, kMaxMatterNodes> log2Ratio {};
    std::array<float, kMaxMatterNodes> weight {};
};

/**
    MATERIAL PROFILE — a material defined entirely by mathematics.

    Every field is a continuous quantity so that two profiles can be blended
    (MaterialMorpher) into a new, consistent material rather than a crossfade
    of two sounds.
*/
struct MaterialProfile
{
    const char* name = "CUSTOM";

    // ---- modal distribution
    FormAnchor ownStructure = FormAnchor::Harmonic; ///< ratio set the material pulls toward
    float structurePull  = 0.0f;   ///< 0..1: how strongly the FORM structure is pulled toward the material's own set
    float stretch        = 0.0f;   ///< inharmonicity B of the material's own set (f_n = n·sqrt(1 + B n²)) when it is stretched-harmonic
    float ratioJitter    = 0.0f;   ///< seeded log2 jitter applied to the material's own set (organic irregularity)
    float ratioSpan      = 1.0f;   ///< multiplies the log2 ratios of the material's own set (compression < 1, expansion > 1)

    // ---- damping curve
    float t60Scale       = 1.0f;   ///< multiplies the global ring time
    float dampingSlope   = 0.6f;   ///< T60 ∝ ratio^(−slope): how much faster high modes die

    // ---- node weighting
    float weightSlope    = 0.0f;   ///< extra amplitude tilt ∝ ratio^(−slope) (negative = brighter)
    float weightRipple   = 0.0f;   ///< seeded random weight variation 0..1

    // ---- coupling topology preference
    float couplingScale  = 0.6f;   ///< multiplies shape.coupling
    float bandBWeight    = 0.4f;   ///< strength of the second (long-range) coupling band relative to the first

    // ---- nonlinearity
    float nonlinearity   = 0.3f;   ///< amplitude-dependent detune / damping (scaled by SURFACE)
    float hardening      = 0.0f;   ///< +1: pitch rises with amplitude (stiff metal), −1: pitch drops (membranes, soft)

    // ---- stereo tendency
    float stereoWidth    = 0.6f;
    StereoPattern stereoPattern = StereoPattern::Random;

    // ---- excitation response
    float exciteTilt     = 0.0f;   ///< −1 dark … +1 bright: pre-filter of the source and per-node excitation tilt
    float strikeContact  = 0.8f;   ///< mallet contact time in ms at MASS 0.4 (longer = softer strike)
    float strikeNoise    = 0.3f;   ///< noise component of the strike 0..1
    float bodyWeight     = 0.2f;   ///< sub-fundamental "body" modes (scaled by MASS)
    float wobble         = 0.0f;   ///< slow random frequency wander depth (log2 units)
    float wobbleRate     = 0.5f;   ///< Hz
    float detune         = 0.01f;  ///< micro detune at SURFACE 1 (log2 units, ±)
    float grain          = 0.3f;   ///< surface grain (state-dependent noise) at SURFACE 1
};

const MaterialProfile& materialProfile (MaterialType type) noexcept;
const char* materialName (MaterialType type) noexcept;

/** Linear interpolation of every continuous field. Enumerated fields follow the dominant profile. */
MaterialProfile blendProfiles (const MaterialProfile& a, const MaterialProfile& b, float t) noexcept;

/**
    Seed-independent anchor structures, built once (call ensureBuilt() from
    prepare(), never from the audio callback the first time).
*/
class StructureLibrary
{
public:
    static const StructureLibrary& get();
    static void ensureBuilt() { (void) get(); }

    const StructureTable& anchor (FormAnchor a) const noexcept { return anchors[(size_t) a]; }

private:
    StructureLibrary();
    std::array<StructureTable, (size_t) FormAnchor::Count> anchors;
};

/** Builds the stochastic FORM anchor for a seed (sorted by construction, no allocation). */
void buildStochasticStructure (uint32_t seed, StructureTable& out) noexcept;

/**
    Builds the material's own structure (the set it pulls the FORM structure
    toward) for a seed. Uses the library anchors plus the profile's stretch,
    jitter and span. Cheap enough for note-on (no sorting).
*/
void buildMaterialStructure (MaterialType type, uint32_t seed, StructureTable& out) noexcept;

} // namespace am
