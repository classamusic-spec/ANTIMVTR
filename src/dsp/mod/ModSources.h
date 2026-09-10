#pragma once

#include "core/Types.h"
#include "core/Random.h"
#include "state/ModRouting.h"

namespace am
{

//==============================================================================
/** LFO shapes — mirrors the `mod.lfoN.shape` choice list. */
enum class LFOShape : uint8_t { Sine = 0, Triangle, Saw, Square, Random, SmoothRandom, Warp, Count };

/** Chaos generator types — mirrors the `mod.chaosN.type` choice list. */
enum class ChaosType : uint8_t { Walk = 0, Brownian, Logistic, Lorenz, Targets, Count };

/** Musical length of one `mod.lfoN.division` choice, in beats (quarter notes). */
double lfoDivisionBeats (int divisionIndex) noexcept;

/** Bends a -1…1 source value. `curve` 0 = linear, > 0 = convex, < 0 = concave. Sign preserving. */
float applyModCurve (float v, float curve) noexcept;

/** Presents a source value with the requested polarity (natural polarity in, requested out). */
inline float presentModValue (float v, bool sourceIsBipolar, bool wantBipolar) noexcept
{
    if (sourceIsBipolar == wantBipolar) return v;
    return sourceIsBipolar ? (v * 0.5f + 0.5f)     // -1…1  ->  0…1
                           : (v * 2.0f - 1.0f);    //  0…1  -> -1…1
}

//==============================================================================
/**
    One low frequency oscillator.

    Free running (global) or per voice: the owner decides whether the phase is
    reset on note-on (`retrigger()`) and whether the phase is locked to the
    host's PPQ position while the transport rolls. Everything is control rate:
    `advance()` moves the oscillator on by a slice of samples and leaves the
    value of the *end* of the slice in `value()`.
*/
class ModLFO
{
public:
    struct Settings
    {
        float    rate      = 1.0f;    ///< Hz (free running)
        LFOShape shape     = LFOShape::Sine;
        bool     sync      = false;
        int      division  = 5;       ///< index into the `mod.lfoN.division` choices
        float    phase     = 0.0f;    ///< 0 … 1 offset
        float    symmetry  = 0.5f;    ///< 0 … 1 skew (0.5 = symmetrical); WARP uses it as the morph
        float    depth     = 1.0f;    ///< 0 … 1 output scale
        bool     retrig    = true;
        float    fade      = 0.0f;    ///< fade-in seconds after a retrigger
    };

    /** Reads the settings of LFO slot `index` (0-based) out of a parameter set. */
    static Settings settingsFor (const ParamValues& params, int index) noexcept;

    /** Effective frequency in Hz for the given settings and transport. */
    static double frequencyFor (const Settings& s, const TransportInfo& t) noexcept;

    void prepare (double sampleRate, uint32_t seed) noexcept;
    void reset() noexcept;

    /** Note-on: phase back to the offset and the fade-in restarted. */
    void retrigger() noexcept;

    /**
        Advances by `numSamples`.

        `lockToTransport` phase-locks a synced LFO to `transport.ppqPosition`
        (which must be the position at the START of the slice); otherwise the
        oscillator free-runs from its own phase.
    */
    void advance (const Settings& s, int numSamples, double sampleRate,
                  const TransportInfo& transport, bool lockToTransport) noexcept;

    /** Current output, -1 … 1, already scaled by depth and the fade-in. */
    float value() const noexcept { return current; }
    /** Raw waveform value before depth / fade, -1 … 1. */
    float shapeValue() const noexcept { return raw; }
    double phase() const noexcept { return ph; }
    float  fadeGain() const noexcept { return fade; }

private:
    float renderShape (const Settings& s, double phase01) noexcept;

    double ph = 0.0;           ///< 0 … 1 running phase (before the offset)
    double lastShapedPhase = 0.0;
    float  current = 0.0f, raw = 0.0f, fade = 1.0f;
    double fadeElapsed = 0.0;
    float  randomA = 0.0f, randomB = 0.0f;   ///< S&H / smooth-random targets
    bool   started = false;
    Rng    rng { 0x5F3A71C1u };
};

//==============================================================================
/**
    Per-voice modulation envelope (ADSR + curve + optional loop).

    Stage progress is linear in time and the *curve* is applied to the shape,
    so a 250 ms attack always takes 250 ms whatever the curve is. Control rate:
    `advance()` steps a whole slice at once. Output is unipolar 0 … 1.
*/
class ModEnvelope
{
public:
    enum class Stage : uint8_t { Idle = 0, Attack, Decay, Sustain, Release };

    struct Settings
    {
        float attack  = 0.01f;
        float decay   = 0.3f;
        float sustain = 0.5f;
        float release = 0.5f;
        float curve   = 0.5f;   ///< 0 = snappy / exponential, 0.5 = linear, 1 = soft
        bool  loop    = false;
    };

    /** Reads the settings of envelope slot `index` (0-based) out of a parameter set. */
    static Settings settingsFor (const ParamValues& params, int index) noexcept;

    void reset() noexcept;
    void noteOn() noexcept;
    void noteOff() noexcept;

    void advance (const Settings& s, int numSamples, double sampleRate) noexcept;

    float value() const noexcept { return level; }
    Stage stage() const noexcept { return st; }
    bool  isActive() const noexcept { return st != Stage::Idle; }

private:
    Stage st = Stage::Idle;
    float level = 0.0f;       ///< 0 … 1 output
    float stagePos = 0.0f;    ///< 0 … 1 progress through the current timed stage
    float releaseFrom = 0.0f;
};

//==============================================================================
/**
    Deterministic chaos generator: random walk, Brownian motion, the logistic
    map, a Lorenz-inspired attractor and interpolated random targets.

    Global (one instance per slot, not per voice), always bounded to -1 … 1 and
    fully reproducible for a given seed.
*/
class ChaosGenerator
{
public:
    struct Settings
    {
        ChaosType type      = ChaosType::Walk;
        float     rate      = 0.5f;   ///< Hz
        float     depth     = 0.5f;
        float     stability = 0.5f;   ///< 0 = wild, 1 = tame
        float     symmetry  = 0.5f;   ///< 0.5 = symmetrical
        uint32_t  seed      = 0;
    };

    /** Reads the settings of chaos slot `index` (0-based) out of a parameter set. */
    static Settings settingsFor (const ParamValues& params, int index) noexcept;

    void prepare (uint32_t seed) noexcept;
    void reset() noexcept;

    void advance (const Settings& s, int numSamples, double sampleRate) noexcept;

    /** Current output, -1 … 1, already scaled by depth. */
    float value() const noexcept { return current; }
    /** Raw generator state before depth, -1 … 1. */
    float rawValue() const noexcept { return state; }

private:
    void step (const Settings& s) noexcept;

    Rng      rng { 1 };
    uint32_t currentSeed = 0xFFFFFFFFu;
    float    state = 0.0f, current = 0.0f;
    float    target = 0.0f, previous = 0.0f;
    float    logistic = 0.5f;
    double   lx = 0.1, ly = 0.0, lz = 20.0;   ///< Lorenz state
    double   phase = 0.0;
};

//==============================================================================
/** The note / MIDI derived source values of one voice, all in their natural polarity. */
struct NoteSourceValues
{
    float velocity   = 0.0f;   ///< 0 … 1
    float keyTrack   = 0.0f;   ///< -1 … 1, centred on C3 (MIDI 60)
    float pressure   = 0.0f;   ///< 0 … 1
    float modWheel   = 0.0f;   ///< 0 … 1
    float pitchBend  = 0.0f;   ///< -1 … 1 (bend / bend range)
    float timbre     = 0.0f;   ///< 0 … 1 (CC74)
    float noteRandom = 0.0f;   ///< -1 … 1, stable for the life of the note
    float gate       = 0.0f;   ///< 1 while the key is held

    float of (ModSource s) const noexcept;

    /** Fills everything except `noteRandom`, which is drawn once at note-on. */
    void update (const NoteState& note, float bendRangeSemis) noexcept;
};

/** MIDI note number the key-tracking source is centred on (C3). */
constexpr int kKeyTrackCentre = 60;

} // namespace am
