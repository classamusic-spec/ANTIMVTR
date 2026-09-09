#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

/**
    Shared fundamental types and constants for the ANTI-MATR engine.

    Everything in namespace `am` is host-agnostic: the core never depends on
    juce_audio_processors or any GUI module so it can be compiled into the
    plugin, the unit tests and the offline tools alike.
*/
namespace am
{

//==============================================================================
// Engine-wide constants

constexpr int kMaxVoices          = 64;    ///< Hard upper bound on polyphony.
constexpr int kDefaultVoices      = 16;
constexpr int kMaxBlockSize       = 4096;  ///< Largest block the engine will ever be asked to render at once.
constexpr int kMaxMatterNodes     = 128;   ///< Upper bound of Matter nodes per voice (ULTRA quality).
constexpr int kMaxFractureFragments = 32;
constexpr int kMaxSequencerSteps  = 32;
constexpr int kNumMacros          = 8;
constexpr int kNumLFOs            = 4;
constexpr int kNumEnvelopes       = 4;
constexpr int kNumChaos           = 4;

constexpr float  kMinFrequencyHz  = 8.0f;
constexpr float  kMaxFrequencyRatio = 0.45f; ///< Fraction of the sample rate any resonator/oscillator may reach.
constexpr float  kSilenceThreshold  = 1.0e-5f;

constexpr double kPi    = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;

//==============================================================================
/** Quality tiers. They scale Matter node counts and oversampling choices. */
enum class Quality : uint8_t { Eco = 0, Normal, High, Ultra, Count };

inline constexpr int matterNodesForQuality (Quality q) noexcept
{
    switch (q)
    {
        case Quality::Eco:    return 32;
        case Quality::Normal: return 64;
        case Quality::High:   return 96;
        case Quality::Ultra:  return 128;
        default:              return 64;
    }
}

/** Public-facing engine stages. Used by diagnostics taps and dry-mode selection. */
enum class Stage : uint8_t
{
    Source = 0,     ///< Raw excitation before Matter
    PostMatter,     ///< After the Matter graph (per-voice, summed)
    PostEvolve,     ///< After Evolve transformations (currently identical tap to PostMatter, kept for future use)
    PostFracture,
    PostSpace,
    Master,
    Count
};

/** Dry-mode selection used by DSP LAB. Production builds leave this at FullSynth. */
enum class DryMode : uint8_t
{
    FullSynth = 0,
    SourceOnly,
    MatterOnly,
    MatterAndEvolve,
    Count
};

/** Subsystems that can be profiled and can raise safety events. */
enum class Subsystem : uint8_t
{
    Source = 0,
    Matter,
    Evolve,
    VoiceMix,
    Fracture,
    Space,
    Modulation,
    Master,
    Visualization,
    State,
    Unknown,
    Count
};

inline const char* subsystemName (Subsystem s) noexcept
{
    switch (s)
    {
        case Subsystem::Source:        return "Source";
        case Subsystem::Matter:        return "Matter";
        case Subsystem::Evolve:        return "Evolve";
        case Subsystem::VoiceMix:      return "VoiceMix";
        case Subsystem::Fracture:      return "Fracture";
        case Subsystem::Space:         return "Space";
        case Subsystem::Modulation:    return "Modulation";
        case Subsystem::Master:        return "Master";
        case Subsystem::Visualization: return "Visualization";
        case Subsystem::State:         return "State";
        default:                       return "Unknown";
    }
}

//==============================================================================
/** Host transport information forwarded to the engine each block. */
struct TransportInfo
{
    double bpm         = 120.0;
    double ppqPosition = 0.0;
    bool   isPlaying   = false;
    int    timeSigNum  = 4;
    int    timeSigDen  = 4;
};

//==============================================================================
/** Per-note state shared by sources, Matter and modulation inside a voice. */
struct NoteState
{
    int    midiNote      = 60;
    float  velocity      = 1.0f;   ///< 0..1
    float  pressure      = 0.0f;   ///< channel or poly aftertouch, 0..1
    float  timbre        = 0.0f;   ///< MPE "slide" (CC74), 0..1
    float  pitchBendSemis = 0.0f;  ///< current bend in semitones (already scaled by range)
    float  modWheel      = 0.0f;   ///< 0..1
    double baseFrequency = 261.6256; ///< Hz, from midiNote + tuning, before bend/glide
    double frequency     = 261.6256; ///< Hz, after bend and glide
    bool   gate          = false;
    bool   sustained     = false;  ///< key released but held by sustain pedal
    uint32_t noteId      = 0;      ///< monotonically increasing per note-on (age ordering)
    int    channel       = 1;
};

//==============================================================================
/** A note number → frequency helper honouring A4 = 440 Hz. */
inline double midiNoteToHz (double note, double a4 = 440.0) noexcept
{
    return a4 * std::pow (2.0, (note - 69.0) / 12.0);
}

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept  { return g > 1.0e-9f ? 20.0f * std::log10 (g) : -180.0f; }

template <typename T>
inline constexpr T clamp01 (T v) noexcept { return v < T (0) ? T (0) : (v > T (1) ? T (1) : v); }

template <typename T>
inline constexpr T lerp (T a, T b, T t) noexcept { return a + (b - a) * t; }

/** Returns true if the value is finite (not NaN, not infinity). */
inline bool isFinite (float v) noexcept { return std::isfinite (v); }

} // namespace am
