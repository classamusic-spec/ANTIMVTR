#pragma once

#include "SourceBase.h"
#include "NoiseGenerators.h"

namespace am
{

/**
    DUST — stochastic excitation (SPEC §11).

    Nine textures share one deterministic random field: the coloured noises
    (WHITE / PINK / BROWN / BLUE), a keytracked resonant band (FILTERED), two
    sparse event engines (CRACKLE, IMPULSE), a granular cloud (CLOUD) and a
    frozen additive texture (FROZEN).

    Every stochastic decision is drawn from `am::Rng` seeded with
    `source.dust.seed` combined with the note id, so a fixed seed and a fixed
    note sequence reproduce the output bit for bit while simultaneous voices
    stay decorrelated.

    Real-time contract: all buffers are sized in prepare(), render() never
    allocates, never locks and never logs.
*/
class DustSource final : public SourceBase
{
public:
    enum class Mode : int
    {
        White = 0, Pink, Brown, Blue, Filtered, Crackle, Impulse, Cloud, Frozen, Count
    };

    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void noteOn (const NoteState& note, const ParamValues& params) override;
    void noteOff() override;
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) override;

    bool  isActive() const noexcept override { return true; }   // continuous source; the amp envelope ends the voice
    float energy() const noexcept override { return lastEnergy; }

private:
    //==========================================================================
    static constexpr int kMaxCrackleEvents = 48;
    static constexpr int kMaxPulses        = 24;
    static constexpr int kMaxGrains        = 32;
    static constexpr int kMaxPartials      = 64;

    /** Cached, per-block parameter snapshot. */
    struct Params
    {
        Mode  mode      = Mode::White;
        float level     = 1.0f;
        float density   = 0.5f;
        float color     = 0.5f;
        float grain     = 0.3f;
        float jitter    = 0.2f;
        float pitchSemis = 0.0f;
        float position  = 0.0f;
        float spread    = 0.5f;
        float stereo    = 0.5f;
        int   seed      = 1;
        double freq     = 261.6256;   ///< keytracked frequency after `pitch`
    };

    /** A decaying crackle grain: only its envelope is stored, the noise is shared. */
    struct CrackleEvent
    {
        bool  active = false;
        float env = 0.0f, attack = 0.0f;
        float decayCoeff = 0.9f, attackCoeff = 0.5f;
        float gainL = 0.0f, gainR = 0.0f;
    };

    /** One windowed impulse of the IMPULSE train (per-channel start offset gives width). */
    struct Pulse
    {
        bool  active = false;
        int   ageL = 0, ageR = 0, len = 1;
        float invLen = 1.0f, gainL = 0.0f, gainR = 0.0f;
    };

    /** One grain of the granular CLOUD. */
    struct Grain
    {
        bool  active = false;
        int   age = 0, len = 1;
        float invLen = 1.0f;
        float phase = 0.0f, inc = 0.0f, rPhase = 0.0f;
        float tone = 0.7f, noise = 0.7f;
        float gainL = 0.0f, gainR = 0.0f;
        Rng   rng { 1 };
    };

    /**
        One FROZEN partial. The seeded fields describe its shape; the running
        state is a quadrature (sin/cos) rotator, which is roughly twice as cheap
        as two windowed table lookups and hands the right channel an arbitrary
        phase offset for free: sin(p + d) = sin p cos d + cos p sin d.
    */
    struct Partial
    {
        float octave = 0.0f;    ///< random offset in octaves around the cluster centre
        float ampRand = 1.0f;
        float pan = 0.0f;
        float rRand = 0.0f;     ///< random right-channel phase offset, 0..1 cycles
        float driftRate = 0.1f;
        float driftPhase = 0.0f;
        float c = 1.0f, s = 0.0f;       ///< rotating unit vector
        float dCos = 1.0f, dSin = 0.0f; ///< per-sample rotation
        float gainL = 0.0f, gainRc = 0.0f, gainRs = 0.0f;
    };

    /** Duty-cycle occupancy used to make `density` audible on the continuous noises. */
    struct DutyGate
    {
        float gain = 1.0f, target = 1.0f, step = 1.0f, maxRamp = 96.0f;
        int   remaining = 0;

        void reset() noexcept { gain = 1.0f; target = 1.0f; remaining = 0; step = 1.0f; }

        inline float next (Rng& rng, float density, float segment, float jitter) noexcept
        {
            if (--remaining <= 0)
            {
                target = rng.nextFloat() < density ? 1.0f : 0.0f;
                const float j = 1.0f + jitter * rng.nextBipolar() * 0.8f;
                const float len = juce::jmax (4.0f, segment * j);
                remaining = (int) len;
                step = 1.0f / juce::jmax (2.0f, juce::jmin (len * 0.25f, maxRamp));
            }
            if (gain < target)      gain = juce::jmin (target, gain + step);
            else if (gain > target) gain = juce::jmax (target, gain - step);
            return gain;
        }
    };

    //==========================================================================
    void readParams (const RenderContext& ctx, const NoteState& note);
    void reseed (uint32_t seedValue, uint32_t noteId);
    void clearEvents();
    void initPartials();

    inline void nextNoise (float& wl, float& wr) noexcept;

    void renderColoured (float* l, float* r, int n);
    void renderFiltered (float* l, float* r, int n);
    void updateFilteredBand();
    void updateFrozen();
    void renderCrackle  (float* l, float* r, int n);
    void renderImpulse  (float* l, float* r, int n);
    void renderCloud    (float* l, float* r, int n);
    void renderFrozen   (float* l, float* r, int n);
    void finalise (float* l, float* r, int n, const RenderContext& ctx);

    //==========================================================================
    double sr = 48000.0;
    Params p;
    Mode   activeMode = Mode::White;

    // Random field ------------------------------------------------------------
    Rng noiseCommon { 1 }, noiseLeft { 2 }, noiseRight { 3 };
    Rng gateRng { 4 }, eventRng { 5 }, grainRng { 6 }, shapeRng { 7 };
    uint32_t baseSeed = 1, currentNoteId = 0;
    int  lastSeedParam = -1;
    float corrCommon = 0.707f, corrIndep = 0.707f;

    // Continuous colouring ----------------------------------------------------
    excitation::PinkFilter       pinkL, pinkR;
    excitation::BrownFilter      brownL, brownR;
    excitation::DifferenceFilter diffL, diffR;
    excitation::TiltFilter       tiltL, tiltR;
    excitation::Svf              bandL, bandR;
    excitation::ExciterLowpass   crackleLpL, crackleLpR;
    excitation::DcBlocker        dcL, dcR;
    DutyGate gate;
    float gateSegment = 400.0f;

    // Control-rate updates. A fixed sample period (not the host block) keeps the
    // time-driven parts of DUST identical whatever buffer size the host uses.
    static constexpr int kControlSamples = 256;
    int  controlCountdown = 0;

    // FILTERED wander ---------------------------------------------------------
    float wanderValue = 0.0f, wanderTarget = 0.0f, wanderPhase = 0.0f;

    // Event engines -----------------------------------------------------------
    std::array<CrackleEvent, kMaxCrackleEvents> crackle {};
    std::array<Pulse,        kMaxPulses>        pulses {};
    std::array<Grain,        kMaxGrains>        grains {};
    std::array<Partial,      kMaxPartials>      partials {};
    double eventCountdown = 0.0;     ///< samples to the next crackle event / cloud grain
    double pulseCountdown = 0.0;     ///< samples to the next impulse-train slot
    int    pulseSlot = 0;
    int    numCrackle = 0, numPulses = 0, numGrains = 0;
    int    numPartials = 32;

    float lastEnergy = 0.0f;
};

} // namespace am
