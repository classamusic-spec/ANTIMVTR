#pragma once

#include "dsp/RenderContext.h"
#include "core/RealtimeHandoff.h"
#include "core/Random.h"
#include "core/Smoothing.h"

#include "Fragment.h"
#include "FractureSequencer.h"
#include "STFT.h"

#include <vector>

namespace am
{

class SafetyMonitor;

//==============================================================================
/**
    FRACTURE — "BREAK INTO NEW REALITIES".

    A post-mix spectral fragmentation engine. The stereo voice mix is analysed
    with a 75 %-overlap STFT; the spectrum is split into 8 / 16 / 32 perceptually
    spaced fragments and each fragment gets its own pitch shift, spectral delay,
    feedback, pan, gain and probability gate. A 1–32 step sequencer drives which
    fragments are open and how they are transposed.

    Modes: SPECTRAL (static), RHYTHMIC (sequencer-gated), TRANSIENT (onset
    triggered) and EVOLVE (settings drift between random targets).

    Latency is *dynamic*: zero while the effect is disengaged (`fracture.on = 0`
    or `fracture.amount * fracture.mix = 0`), where the output is a bit-exact
    copy of the input; `fftSize()` samples while engaged, with the dry path
    running through a matched delay so wet and dry stay time-aligned.
    `latencySamples()` always reports the truth and `latencyChanged()` lets the
    principal forward it to the host. The two dry taps are crossfaded over ~20 ms
    when the effect engages, so toggling never clicks.
*/
class FractureEngine
{
public:
    FractureEngine();
    ~FractureEngine();

    //==========================================================================
    /** Per-fragment state for DSP LAB / visualisation. Copyable, plain data. */
    struct FragmentActivity
    {
        int   numFragments = 0;
        int   currentStep  = 0;
        float overall      = 0.0f;   ///< 0 … 1 wet share of the output energy
        std::array<float, kMaxFractureFragments> gain {};    ///< smoothed fragment gate, 0 … 1
        std::array<float, kMaxFractureFragments> energy {};  ///< normalised wet energy, 0 … 1
    };

    //==========================================================================
    // Audio thread
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (float* l, float* r, int n, const RenderContext& ctx);

    /** Latency the engine is adding right now: 0 when disengaged, `fftSize()` when engaged. */
    int   latencySamples() const noexcept { return reportedLatency.load (std::memory_order_relaxed); }

    /** The latency the engine reaches when engaged (== `fftSize()`). Constant after prepare(). */
    int   maxLatencySamples() const noexcept { return latency; }

    /** True once after `latencySamples()` changed — poll from the message thread and
        forward the new value to the host with `setLatencySamples()`. */
    bool  latencyChanged() noexcept { return latencyDirty.exchange (false, std::memory_order_acq_rel); }

    float activity() const noexcept       { return activityValue.load (std::memory_order_relaxed); }
    int   fftSize() const noexcept        { return stft.size(); }
    int   hopSize() const noexcept        { return stft.hop(); }

    //==========================================================================
    // Message thread
    /** Frees fragment tables the audio thread has finished with. Call from a timer. */
    void messageThreadMaintenance() { handoff.collectGarbage(); }

    /** Hands a new fragment table / sequencer pattern to the audio thread. */
    void publishTable (std::unique_ptr<FractureTable> newTable) { handoff.publish (std::move (newTable)); }

    /** Restarts the sequencer on the next block when `fracture.retrig` is on.
        Safe from any thread; the principal calls it on the first note of a phrase. */
    void noteStarted() noexcept { retrigRequest.store (true, std::memory_order_release); }

    /** Copies the current per-fragment activity (any thread; wait-free). */
    void fillFragmentActivity (FragmentActivity& out) const noexcept;

private:
    //==========================================================================
    void processFrame (float* specL, float* specR, int bins);
    void onNewStep();
    void updateFragments();
    void detectOnset (const float* specL, const float* specR, int bins);
    void rebuildBands (int bins);
    void rebuildScatter (uint32_t seed);
    void rebuildTilt (float tone, int bins);
    int  ringIndex (int frame) const noexcept { return ((frame % maxFrames) + maxFrames) % maxFrames; }

    //==========================================================================
    /** Everything `processFrame` needs, resolved once per block. */
    struct BlockParams
    {
        FractureMode mode = FractureMode::Spectral;
        int   fragments   = 16;
        float spread      = 0.3f;
        float sequence    = 0.5f;
        float feedback    = 0.2f;
        float pitch       = 0.0f;
        float delayScale  = 0.3f;
        float decay       = 0.5f;
        float evolve      = 0.0f;
        float probability = 1.0f;
        uint32_t seed     = 11;
    };

    STFT stft;
    FractureSequencer seq;
    FractureSequencer::Settings seqSettings;
    BlockParams bp;

    RealtimeHandoff<FractureTable> handoff;
    FractureTable defaults = FractureTable::makeDefault();
    const FractureTable* table = &defaults;

    // --- spectral state -------------------------------------------------
    std::vector<float> ring[2];        ///< maxFrames × (2 · maxBins) interleaved complex frames
    std::vector<float> wetSpec[2];     ///< un-panned wet accumulator, 2 · maxBins
    std::vector<float> synMag[2], synFreq[2], sumPhase[2];
    std::vector<float> panGainL, panGainR, fbGainBin, tilt, melPos, prevMag;
    std::array<int, kMaxFractureFragments + 1> bandEdge {};

    int maxBins = 513, binStride = 1026, maxFrames = 192, writeFrame = 0;
    float radiansPerBin = 0.0f;        ///< 2π · hop / fftSize
    float binsPerRadian = 0.0f;        ///< fftSize / (2π · hop)

    // --- per-fragment runtime state -------------------------------------
    std::array<float, kMaxFractureFragments> fragGain {}, fragGainTarget {};
    std::array<float, kMaxFractureFragments> fragPitch {}, fragPitchTarget {};
    std::array<float, kMaxFractureFragments> fragPan {}, fragPanTarget {};
    std::array<float, kMaxFractureFragments> fragFb {}, fragFbTarget {};
    std::array<float, kMaxFractureFragments> fragDelay {}, fragDelayTarget {};
    std::array<float, kMaxFractureFragments> scatterPitch {}, scatterDelay {}, scatterPan {};
    std::array<float, kMaxFractureFragments> evolvePitch {}, evolveDelay {}, evolvePan {}, evolveGain {};
    std::array<float, kMaxFractureFragments> targetPitch {}, targetDelay {}, targetPan {}, targetGain {};
    std::array<float, kMaxFractureFragments> transientEnv {};
    std::array<float, kMaxFractureFragments> fragEnergy {};
    std::array<bool,  kMaxFractureFragments> probPass {};

    // --- dry path -------------------------------------------------------
    std::vector<float> dryLine[2], inScratch[2], wetScratch[2];
    int dryMask = 0, dryWrite = 0;
    OnePoleSmoother blendSmooth, tapSmooth;

    // --- misc -----------------------------------------------------------
    Rng rng { 11 };
    double sr = 48000.0;
    int   latency = 1024;
    int   activeFragments = 16;
    int   bandsBuiltFor = 0;
    uint32_t scatterSeed = 0xFFFFFFFFu;
    float tiltBuiltFor = -1.0f;
    float onsetFast = 0.0f, onsetSlow = 0.0f;
    int   framesSinceOnset = 1000;
    bool  wasPlaying = false;

    SafetyMonitor* safety = nullptr;   ///< borrowed from the render context each block

    std::atomic<bool>  retrigRequest { false };
    std::atomic<bool>  latencyDirty { false };
    std::atomic<int>   reportedLatency { 0 };
    std::atomic<float> activityValue { 0.0f };
    std::atomic<int>   actFragments { 16 }, actStep { 0 };
    std::array<std::atomic<float>, kMaxFractureFragments> actGain {}, actEnergy {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractureEngine)
};

} // namespace am
