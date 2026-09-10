#pragma once

#include "SpacePresets.h"
#include "Chorus.h"
#include "Compressor.h"
#include "Delay.h"
#include "Distortion.h"
#include "EQ.h"
#include "FrequencyShifter.h"
#include "GranularDelay.h"
#include "Limiter.h"
#include "Reverb.h"
#include "SpectralDiffusion.h"
#include "dsp/RenderContext.h"

namespace am
{

/**
    THE RACK — every SPACE module, in the order the current Space asks for.

    The rack renders the *wet* path only; SpaceEngine owns the dry signal and
    the global mix. Responsibilities:

      * translate rack parameters + the SIZE / TONE / FEEDBACK macros into
        module settings once per block,
      * run the modules in the order given by `SpacePresets::routing()`,
      * crossfade each module in and out over ~8 ms so toggling one mid-note
        is inaudible, and release its tail once it has been off for a moment,
      * apply the TONE tilt to the wet signal just before the utility modules
        (EQ, compressor, limiter), and
      * report whether any feedback path had to be clamped.
*/
class FXRack
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Processes the wet buffers in place. `n` must be <= maxBlockSize. */
    void process (float* l, float* r, int n, const RenderContext& ctx, const SpacePresets::Routing& routing);

    /**
        The rack limiter, applied by SpaceEngine to the *mixed* Space output.

        It sits after the dry/wet mix rather than inside the wet chain: a
        limiter at the end of a rack exists to stop the rack from throwing
        anything nasty at the next stage, and dry + wet is what leaves SPACE.
        The master's safety limiter therefore never has to work for SPACE.
    */
    void processOutput (float* l, float* r, int n);

    bool feedbackClamped() const noexcept { return clamped; }
    float reverbRT60() const noexcept { return reverb.currentRT60(); }

    /** Musical scaling factors derived from the macros (also used by the tests). */
    static float sizeScale (float size01) noexcept { return std::pow (2.0f, (fx::clampf (size01, 0.0f, 1.0f) - 0.5f) * 1.6f); }
    static float toneTilt (float tone01) noexcept  { return (fx::clampf (tone01, 0.0f, 1.0f) - 0.5f) * 2.0f; }

private:
    void updateParameters (const RenderContext& ctx, const SpacePresets::Routing& routing);
    void runSlot (SpacePresets::Slot slot, float* l, float* r, int n);
    void processSlot (SpacePresets::Slot slot, float* l, float* r, int n);
    void resetSlot (SpacePresets::Slot slot);
    void applyTilt (float* l, float* r, int n);

    static constexpr int kNumSlots = SpacePresets::kNumSlots;
    static constexpr int kIdleBlocksBeforeReset = 64;   ///< ~170 ms at 48 k / 128

    fx::Distortion       distortion;
    fx::Chorus           chorus;
    fx::FrequencyShifter shifter;
    fx::Delay            delay;
    fx::GranularDelay    granular;
    fx::SpectralDiffusion diffusion;
    fx::Reverb           reverb;
    fx::EQ               eq;
    fx::Compressor       compressor;
    fx::Limiter          limiter;
    fx::TiltFilter       tilt;

    std::array<fx::ModuleGate, kNumSlots> gates;
    std::array<int, kNumSlots> idleBlocks {};

    std::vector<float> tmpL, tmpR;
    double sr = 48000.0;
    int maxBlock = 512;
    float currentTilt = 0.0f;
    bool tiltActive = false;
    bool clamped = false;
    bool delayNeedsSnap = true;
    bool firstUpdate = true;
};

} // namespace am
