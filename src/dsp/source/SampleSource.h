#pragma once

#include "SourceBase.h"
#include "SampleData.h"
#include "NoiseGenerators.h"

namespace am
{

/**
    SAMPLE — imported audio as energy (SPEC §13).

    The audio itself lives in an immutable `SampleData` owned by the message
    thread and published through `SynthEngine`'s `RealtimeHandoff`; every voice
    reads the pointer the engine put into `RenderContext::sample` for the
    block, so switching sample never allocates, locks or frees on the audio
    thread.

    Four modes:
      * ONE SHOT — plays start → end once.
      * LOOP     — equal-power crossfaded loop between start and end; the
                   crossfade makes the seam continuous whatever the content.
      * REVERSE  — plays end → start once.
      * GRANULAR — an asynchronous cloud of up to 16 Hann-windowed grains.
                   Grain size comes from `grain` (5..500 ms, exponential),
                   the read scatter from `spread`; the cloud centre advances
                   at the sample's natural speed while the grains are read at
                   the playback ratio, so pitch and time stay independent.

    Pitch = root note + keytrack + `pitch` semitones. Reads use a four point
    cubic Hermite interpolator; transposing up additionally averages up to
    four evenly spaced reads across the step, which is a cheap decimation
    filter that keeps the top octaves from folding.
*/
class SampleSource final : public SourceBase
{
public:
    enum class Mode : int { OneShot = 0, Loop, Reverse, Granular, Count };

    static constexpr int kMaxGrains = 16;

    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void noteOn (const NoteState& note, const ParamValues& params) override;
    void noteOff() override;
    void render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note) override;

    bool  isActive() const noexcept override;
    float energy() const noexcept override { return lastEnergy; }

    /** Playback position in frames (diagnostics / tests). */
    double position() const noexcept { return pos; }
    /** Grains currently sounding (GRANULAR only). */
    int activeGrains() const noexcept { return numGrains; }

private:
    struct Params
    {
        Mode  mode      = Mode::OneShot;
        float level     = 1.0f;
        float start     = 0.0f;
        float end       = 1.0f;
        float grain     = 0.3f;
        float spread    = 0.3f;
        float pitchSemis = 0.0f;
        int   root      = 60;
        bool  keytrack  = true;
    };

    /** One grain of the cloud: a windowed read head with its own rate and pan. */
    struct Grain
    {
        bool   active = false;
        double position = 0.0;
        double rate = 1.0;
        int    age = 0, length = 1;
        float  invLength = 1.0f;
        float  gainL = 0.0f, gainR = 0.0f;
    };

    void  readParams (const RenderContext& ctx, const NoteState& note);
    void  updateBounds();
    void  restart();
    void  spawnGrain();
    inline float readAt (int channel, double position, double rate) const noexcept;
    inline void  readStereo (double position, double rate, float& outL, float& outR) const noexcept;
    void  renderLinear (float* l, float* r, int n);
    void  renderGranular (float* l, float* r, int n);
    void  finalise (float* l, float* r, int n, const RenderContext& ctx);

    double sr = 48000.0;
    Params p;
    const SampleData* sample = nullptr;   ///< valid for the current block only

    // Playback -----------------------------------------------------------------
    double pos = 0.0;            ///< current read position in frames
    double rate = 1.0;           ///< frames advanced per output sample (sign = direction)
    double startFrame = 0.0, endFrame = 0.0, crossfade = 0.0;
    double cloudPos = 0.0;       ///< GRANULAR: centre of the grain cloud
    double grainCountdown = 0.0;
    bool   finished = false, gate = false;
    int    interpTaps = 1;

    // Grains -------------------------------------------------------------------
    std::array<Grain, kMaxGrains> grains {};
    int numGrains = 0;
    float grainGain = 1.0f;
    Rng   grainRng { 0x5A11u };
    uint32_t noteId = 0;

    // Click guards --------------------------------------------------------------
    // A sample swap or a note start fades the new signal in while the last emitted
    // value decays away, so neither can produce a step.
    static constexpr double kFadeSeconds = 0.004;
    float residueL = 0.0f, residueR = 0.0f, residueDecay = 0.99f;
    float lastOutL = 0.0f, lastOutR = 0.0f;
    float fadeGain = 1.0f, fadeStep = 1.0f;

    excitation::DcBlocker dcL, dcR;
    float lastEnergy = 0.0f;
};

} // namespace am
