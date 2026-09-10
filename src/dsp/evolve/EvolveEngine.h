#pragma once

#include "dsp/matter/MatterEngine.h"
#include "core/Random.h"

namespace am
{

/**
    EVOLVE ENGINE — transformations applied directly to the Matter nodes
    (partial-domain processing, SPEC §26–32): BEND, MELT, TEAR, MAGNET,
    GRAVITY, SCATTER, FREEZE and CRUSH.

    apply() runs once per block per voice right before MatterEngine::process.
    It reads the material baseline of every node (`targetFrequency`, `ratio`,
    `weight`, `damping`, `pan`, `nonlinearity`, `excitation`, `energy`,
    `cluster`, `active`) and writes `frequency`, `weight`, `damping`, `pan`,
    `nonlinearity` and `excitation` for the coming block only: Matter rewrites
    the baseline at the end of every block, so the operators are re-applied
    every block from scratch and never accumulate (see MatterNode.h).

    Every operator is always live at its own amount; `evolve.selected` is a
    UI concern. Operators at zero write nothing, so a patch with all amounts
    at zero leaves the nodes bit-identical.

    Frequency work happens in the log2 domain relative to the material
    fundamental f0 (node 0's target / ratio). Each node has a spectral height
    h = log2 (f / f0) / 4 (0 at the fundamental, 1 from four octaves up) that
    BEND, MELT, GRAVITY and SCATTER use to weigh their effect by rank; a fixed
    span keeps the operators object-independent and predictable.

      BEND     lever around a pivot: partials above it rise, below it sink,
               up to ±bendRange octaves, distributed by bendCurve.
      MELT     partials sag downward with rank, damp faster, high weights fade.
      TEAR     the least important active nodes become detuned twins of the
               most important ones (beating pairs); the remaining clusters are
               pulled apart in pitch, stereo and decay; nonlinearity rises.
      MAGNET   pulls log-frequencies toward a grid (OCTAVE, FIFTH, MAJOR,
               MINOR, CHROMATIC, SCALE = major pentatonic, CUSTOM = the
               material's ratios rounded to integers, i.e. a harmonic series).
      GRAVITY  0.5 neutral; below it weight, excitation and ring time lift
               toward the top partials, above it everything sinks. The side
               it pushes away from also loses ring time, and the weights are
               compensated back towards the drive the node set had before, so
               the control tilts the spectrum instead of changing the level.
      SCATTER  seeded per-node offsets of frequency, weight and pan
               (scatterSeed + noteId), animated by SPEED/MOTION.
      FREEZE   captures the node state, floors the damping so the object rings
               indefinitely, stops the motion clock, reduces new excitation.
      CRUSH    quantises log-frequencies to a coarse grid and weights to a few
               levels, silences the quietest nodes, adds nonlinearity.
      SPEED    rate of the internal motion clock (0.02–8 Hz, exponential).
      MOTION   amount of slow per-node drift applied to the active operators.

    Real-time safe: no allocation after prepare, O(N) per block, seeded Rng.
*/
class EvolveEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void noteOn (const NoteState& note, const ParamValues& params);

    /** Applies the operators to the Matter graph for the coming block. */
    void apply (MatterEngine& matter, const RenderContext& ctx, const NoteState& note);

    /** Copies what the last apply() did into the diagnostics record. */
    void fillDiagnostics (EvolveDiag& dest) const noexcept;

    // ---- introspection (tests, DSP LAB)
    float motionPhase() const noexcept { return (float) phase; }
    bool isFrozen() const noexcept { return frozen; }

    /** Motion clock rate for a SPEED value in 0..1 (0.02 Hz … 8 Hz, exponential). */
    static float motionRateHz (float speed) noexcept;

    /**
        Nearest point of a MAGNET grid to `log2Ratio` (log2 of frequency / fundamental)
        for a target index in evolve.magnetTarget choice order, in the same units.
    */
    static float magnetGridLog2 (int target, float log2Ratio) noexcept;

    /** CRUSH frequency grid step in octaves for an amount in 0..1. */
    static float crushGridOctaves (float amount) noexcept;

    /** Damping FREEZE pushes toward (per-sample amplitude loss) at a sample rate. */
    static float freezeDamping (double sampleRate) noexcept;

private:
    struct Amounts
    {
        float bend = 0.0f, melt = 0.0f, tear = 0.0f, magnet = 0.0f, gravity = 0.0f, scatter = 0.0f, crush = 0.0f;
        float speed = 0.3f, motion = 0.0f, bendPivot = 0.5f, bendRange = 0.5f, bendCurve = 0.5f;
        int   magnetTarget = 1;
        uint32_t scatterSeed = 0, mask = 0;
        bool  freeze = false;
    };

    static Amounts readAmounts (const RenderContext& ctx) noexcept;
    void buildScatterTables (uint32_t seed) noexcept;
    void buildMotionTables (uint32_t seed) noexcept;

    double sr = 48000.0;
    double phase = 0.0;             ///< motion clock, 0..1
    float  rateHz = 0.0f;
    uint32_t noteId = 0;
    uint32_t scatterSeedUsed = 0xFFFFFFFFu;
    bool   frozen = false;
    int    frozenCount = 0;

    // Seeded per-node tables (rebuilt at note-on / seed change, never per block).
    std::array<float, kMaxMatterNodes> scatterFreq {}, scatterWeight {}, scatterPan {};
    std::array<float, kMaxMatterNodes> motionOffsetA {}, motionOffsetB {};

    // Per-block scratch (no allocation).
    std::array<float, kMaxMatterNodes> logRatio {}, height {}, shiftOct {}, lfo {};
    std::array<float, kMaxMatterNodes> weight {}, damping {}, pan {}, excitation {}, nonlinearity {};
    std::array<uint8_t, kMaxMatterNodes> activeIndex {};
    std::array<bool, kMaxMatterNodes> considered {};

    // FREEZE capture
    std::array<float, kMaxMatterNodes> frozenFreq {}, frozenWeight {}, frozenPan {}, frozenNonlinearity {};

    // CRUSH envelope hold (digital decay)
    std::array<float, kMaxMatterNodes> lastWrittenWeight {}, crushHold {}, crushAmp {}, crushEnergySeen {}, crushGain {};
    std::array<uint8_t, kMaxMatterNodes> crushState {};    ///< 0 fresh, 1 holding a level, 2 cut
    float  crushPeak = 0.0f;
    double crushClock = 0.0;
    bool   crushArmed = false;

    EvolveDiag diag;
};

} // namespace am
