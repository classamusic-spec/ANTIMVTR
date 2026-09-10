#pragma once

#include "SourceBase.h"
#include "NoiseGenerators.h"

namespace am
{

/**
    GESTURE — continuous, physically inspired excitation (SPEC §14).

    Where IMPACT hits Matter once, GESTURE keeps pushing energy into it: a bow
    that sticks and slips, a scraped surface, a slow rub, breath through a
    tube, dry friction and electrical sputter. Every mode is band limited, DC
    free and deterministic (seeded `am::Rng`), gated by the note with a short
    release so releasing a key stops the gesture without a click.

    Shared controls:
      * `pressure`  — how hard the gesture presses: grip and harmonic richness
                      plus (always monotonically) level.
      * `speed`     — how fast it moves: event density, turbulence and level.
      * `roughness` — noise inside the friction and irregularity of its timing.
      * `position`  — contact point: a feed-forward comb tuned to the note
                      (0.5 is neutral, either side colours the excitation).
      * `motion`    — slow seeded drift of pressure and speed.
      * `bandwidth` — resonant band-pass around the note, narrow (0) to wide (1).

    Real-time contract: everything is sized in prepare(); render() never
    allocates, locks or logs.
*/
class GestureSource final : public SourceBase
{
public:
    enum class Mode : int { Bow = 0, Scrape, Rub, Breath, Friction, Electrical, Count };

    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void noteOn (const NoteState& note, const ParamValues& params) override;
    void noteOff() override;
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) override;

    bool  isActive() const noexcept override { return env > 1.0e-4f; }
    float energy() const noexcept override { return lastEnergy; }

private:
    static constexpr int kCombSize   = 2048;   ///< contact-point comb, ~21 ms at 96 kHz
    static constexpr int kMaxGrains  = 12;     ///< SCRAPE friction grains
    static constexpr int kControlSamples = 32; ///< control-rate update period

    struct Params
    {
        Mode  mode      = Mode::Bow;
        float level     = 1.0f;
        float pressure  = 0.5f;
        float speed     = 0.5f;
        float roughness = 0.3f;
        float position  = 0.3f;
        float motion    = 0.2f;
        float bandwidth = 0.5f;
        float velocity  = 1.0f;
        double freq     = 261.6256;
    };

    /** One friction grain: an envelope only, the noise field is shared. */
    struct Grain
    {
        bool  active = false;
        float env = 0.0f, decay = 0.9f, amp = 0.0f;
    };

    void readParams (const RenderContext& ctx, const NoteState& note);
    void updateControl();
    void updateFilters();

    inline float renderBow() noexcept;
    inline float renderScrape() noexcept;
    inline float renderRub() noexcept;
    inline float renderBreath() noexcept;
    inline float renderFriction() noexcept;
    inline float renderElectrical() noexcept;

    inline float applyComb (float x) noexcept;
    void finalise (float* l, float* r, int n, const RenderContext& ctx);

    double sr = 48000.0;
    Params p;

    // Envelope -----------------------------------------------------------------
    float env = 0.0f, attackCoeff = 0.01f, releaseCoeff = 0.001f;
    bool  gate = false;

    // Modulated (motion) control values ---------------------------------------
    float pressureEff = 0.5f, speedEff = 0.5f;
    float motionA = 0.0f, motionB = 0.0f, motionTargetA = 0.0f, motionTargetB = 0.0f;
    int   motionHold = 0, controlCountdown = 0;

    // Shared random field ------------------------------------------------------
    Rng   noiseRng { 0x9E37u }, eventRng { 0x1234u }, shapeRng { 0xABCDu };
    uint32_t noteId = 0;

    // BOW ----------------------------------------------------------------------
    double bowPhase = 0.0, bowInc = 0.0;
    float  bowGrip = 0.6f, bowLp = 0.0f, bowLpCoeff = 0.3f, bowSlipNoise = 0.0f;

    // SCRAPE / FRICTION --------------------------------------------------------
    std::array<Grain, kMaxGrains> grains {};
    double eventCountdown = 0.0;
    float  grainSum = 0.0f;

    // RUB ----------------------------------------------------------------------
    float rubPhase = 0.0f, rubInc = 0.0f;

    // BREATH -------------------------------------------------------------------
    float turbulence = 0.0f, turbulenceTarget = 0.0f;
    int   turbulenceHold = 0;

    // ELECTRICAL ---------------------------------------------------------------
    double pulseCountdown = 0.0;
    float  sputter = 1.0f;

    // Filters ------------------------------------------------------------------
    excitation::PinkFilter       pink;
    excitation::TiltFilter       tilt;
    excitation::Svf              band, formant1, formant2, tone;
    excitation::ExciterLowpass   pulseLp, breathLp;
    // Each spark leaves the low pass with unit area; without its own high pass the
    // random walk of a sparse pulse train shows up as sub-audio wander.
    excitation::DcBlocker        pulseDc, dcL, dcR;
    float bandGain = 1.0f, dryMix = 0.0f, modeTrim = 1.0f;
    bool  tonalBand = false;   ///< pitched modes normalise the band by its peak gain, noisy ones by its noise gain

    // Contact-point comb --------------------------------------------------------
    std::array<float, kCombSize> comb {};
    int   combWrite = 0, combDelay = 0;
    float combDepth = 0.0f;

    float lastEnergy = 0.0f;
};

} // namespace am
