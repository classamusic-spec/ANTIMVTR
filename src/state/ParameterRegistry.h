#pragma once

#include "ParameterList.h"
#include "core/Types.h"

#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace am
{

enum class ParamKind : uint8_t { Float, Bool, Choice, Int };

enum class ParamGroup : uint8_t
{
    Master, Amp, Source, Wave, Dust, Impact, Sample, Gesture,
    Shape, Evolve, Fracture, Space, Mod, Macro, Count
};

/** DNA categories used by the mutation engine. */
enum class MutationCategory : uint8_t
{
    None, Source, Shape, Evolve, Fracture, Movement, Space, Pitch, Chaos, Count
};

enum class SmoothingKind : uint8_t { None, Fast, Medium, Slow };

//==============================================================================
/** The permanent parameter enumeration, generated from ParameterList.h. */
enum class Param : uint16_t
{
   #define AM_PARAM_ENUM(e, id, name, grp, kind, mn, mx, df, skew, unit, choices, mod, mut, smooth) e,
    ANTIMATR_PARAMETER_LIST (AM_PARAM_ENUM)
   #undef AM_PARAM_ENUM
    Count
};

constexpr int kNumParams = static_cast<int> (Param::Count);

inline constexpr int paramIndex (Param p) noexcept { return static_cast<int> (p); }
inline constexpr Param paramFromIndex (int i) noexcept { return static_cast<Param> (i); }

/** Dense array of parameter values indexed by Param. Values are in their natural (denormalised) range. */
using ParamValues = std::array<float, kNumParams>;

//==============================================================================
/** Static description of one parameter. */
struct ParamDesc
{
    Param            param;
    const char*      id;          ///< permanent dotted identifier
    const char*      name;        ///< customer-facing label
    ParamGroup       group;
    ParamKind        kind;
    float            min;
    float            max;
    float            defaultValue;
    float            skew;        ///< NormalisableRange skew (1 = linear)
    const char*      unit;
    const char*      choices;     ///< '|' separated list for Choice kind, else ""
    bool             modulatable;
    MutationCategory mutation;
    SmoothingKind    smoothing;

    /** True for parameters the engine reads once per block, after the voices are summed
        (the Fracture, Space and Master stages, and the voice allocator's own settings).
        A per-voice modulation source cannot reach these through a voice's parameter copy. */
    bool  isPostVoice() const noexcept { return group == ParamGroup::Fracture || group == ParamGroup::Space || group == ParamGroup::Master; }

    int   numChoices() const noexcept;
    bool  isDiscrete() const noexcept { return kind != ParamKind::Float; }
    float clampValue (float v) const noexcept;

    /** Convert between natural value and normalised 0..1 (same curve the host sees). */
    float toNormalised (float value) const noexcept;
    float fromNormalised (float norm) const noexcept;

    /** Human readable value (e.g. "0.62", "-6.0 dB", "MEMBRANE"). */
    juce::String formatValue (float value) const;
};

//==============================================================================
/** Read-only registry of every parameter descriptor. */
class ParameterRegistry
{
public:
    static const ParamDesc& get (Param p) noexcept;
    static const std::array<ParamDesc, kNumParams>& all() noexcept;

    /** Looks up a parameter by its permanent ID. */
    static std::optional<Param> fromID (std::string_view id) noexcept;

    /** Fills an array with every parameter's default value. */
    static void fillDefaults (ParamValues& values) noexcept;
    static ParamValues defaults() noexcept;

    /** Returns the descriptors belonging to a group. */
    static std::vector<Param> inGroup (ParamGroup g);

    static const char* groupName (ParamGroup g) noexcept;
    static const char* mutationName (MutationCategory m) noexcept;

    /** Smoothing time in milliseconds for a smoothing kind. */
    static float smoothingMs (SmoothingKind k) noexcept;

    /** Validates the table at startup (unique IDs, defaults in range). Returns an empty string when OK. */
    static juce::String validate();
};

inline float paramValue (const ParamValues& v, Param p) noexcept { return v[(size_t) paramIndex (p)]; }
inline int   paramChoice (const ParamValues& v, Param p) noexcept { return (int) std::lround (v[(size_t) paramIndex (p)]); }
inline bool  paramBool (const ParamValues& v, Param p) noexcept { return v[(size_t) paramIndex (p)] >= 0.5f; }

} // namespace am
