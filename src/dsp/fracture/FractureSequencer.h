#pragma once

#include "Fragment.h"
#include "core/Random.h"

namespace am
{

//==============================================================================
/**
    The FRACTURE fragment sequencer: 1 … 32 steps, tempo-synced or free running,
    swing, four directions, per-step probability and randomisation.

    It is driven at hop granularity by `FractureEngine` (one `advance()` per STFT
    frame), so gate edges land within one hop of their ideal position. Everything
    is deterministic for a given seed; no allocation, no locks.
*/
class FractureSequencer
{
public:
    struct Settings
    {
        int               numSteps    = 8;
        bool              sync        = true;
        int               division    = 3;      ///< index into `fracture.division`
        float             rateHz      = 2.0f;   ///< used when `sync` is false
        float             swing       = 0.0f;   ///< 0 … 1
        FractureDirection direction   = FractureDirection::Forward;
        float             probability = 1.0f;   ///< global, multiplied with the step's own
        float             randomAmount = 0.0f;  ///< 0 … 1 per-step randomisation
        uint32_t          seed        = 11;
        double            bpm         = 120.0;
    };

    /** The resolved values of the step that is currently playing. */
    struct StepState
    {
        int      index  = 0;
        uint32_t mask   = 0xFFFFFFFFu;
        float    gate   = 1.0f;
        float    pitch  = 0.0f;
        float    pan    = 0.0f;
        float    gain   = 1.0f;
        float    evolve = 0.0f;
        float    shape  = 0.0f;
        bool     active = true;   ///< false when the probability roll failed
    };

    void prepare (double sampleRate);
    void reset (uint32_t seed);

    /** Jumps back to the first step of the pattern and re-seeds the random stream. */
    void restart() noexcept;

    /** Advances the clock by `samples`; returns true if a new step started. */
    bool advance (int samples, const Settings& s, const FractureTable& table);

    const StepState& state() const noexcept { return current; }
    int    stepIndex() const noexcept       { return current.index; }
    float  phase() const noexcept           { return stepLength > 0.0 ? (float) (position / stepLength) : 0.0f; }
    double stepLengthSamples() const noexcept { return stepLength; }
    uint32_t stepCount() const noexcept     { return steppedCount; }

    /** Stable per-step seed so callers can roll their own deterministic decisions. */
    uint32_t stepSeed() const noexcept { return hashSeed (currentSeed, steppedCount * 2654435761u); }

    /** Number of samples per step for the given settings (test / diagnostics helper). */
    static double stepSamples (const Settings& s, double sampleRate) noexcept;

    /** Beat multiplier for a `fracture.division` index (1.0 = one quarter note). */
    static double divisionBeats (int division) noexcept;

private:
    void beginStep (const Settings& s, const FractureTable& table);

    double sr = 48000.0;
    double position = 0.0;      ///< samples into the current step
    double stepLength = 12000.0;
    StepState current;
    Rng rng { 11 };
    uint32_t currentSeed = 11;
    uint32_t steppedCount = 0;
    int cursor = 0;
    int pingPongDir = 1;
    bool needsFirstStep = true;
};

} // namespace am
