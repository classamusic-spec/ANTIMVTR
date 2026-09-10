#pragma once

#include "FXRack.h"
#include "dsp/RenderContext.h"

namespace am
{

/**
    SPACE — "place it anywhere".

    The engine owns the dry signal, the wet rack and the four macros:

      MIX       equal-power dry/wet. At 0 the output is the input, sample for
                sample, and the rack is skipped entirely (and released after a
                moment) so an unused SPACE costs nothing.
      SIZE      stretches delay times, room size, grain length and chorus
                depth together — one control for "how big is this place".
      TONE      spectral tilt of the wet signal plus damping tendencies:
                dark cave at 0, bright glass at 1.
      FEEDBACK  scales every regeneration path (delay, granular, reverb decay)
                and is always clamped below unity; when a request has to be
                limited, SafetyEvent::FeedbackClamp is raised.

    The module order comes from `SpacePresets::routing (space.type)` and is
    read at the start of every block; when it changes, the wet path is faded
    down and back up over a few milliseconds so a Space change never clicks.
*/
class SpaceEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (float* l, float* r, int n, const RenderContext& ctx);

    /**
        Wet energy as a fraction of the Space stage's output energy, 0..1 —
        the UI's SPACE halo. 0 when fully dry, 1 when fully wet, and in
        between it follows both the MIX macro and how much the rack is
        actually doing. Lightly smoothed across blocks so the halo breathes
        instead of flickering.
    */
    float activity() const noexcept { return lastActivity; }

    /** SPACE adds no latency: the limiter is zero-latency by design. */
    int latencySamples() const noexcept { return 0; }

    /** Message thread: frees data handed off to the audio thread (see core/RealtimeHandoff.h). */
    void messageThreadMaintenance() {}

    /** Diagnostics / tests. */
    float reverbRT60() const noexcept { return rack.reverbRT60(); }
    bool  feedbackClamped() const noexcept { return rack.feedbackClamped(); }

private:
    void processChunk (float* l, float* r, int n, const RenderContext& ctx);

    FXRack rack;
    std::vector<float> dryL, dryR;

    LinearRamp dryRamp, wetRamp;
    SpacePresets::Routing routing, pendingRouting;
    bool routingChanging = false, routingInitialised = false, firstMixBlock = true;
    float changeGain = 1.0f, changeTarget = 1.0f, changeStep = 0.01f;

    double sr = 48000.0;
    int maxBlock = 512;
    int idleBlocks = 0;
    double blockDryEnergy = 0.0, blockWetEnergy = 0.0;
    float lastActivity = 0.0f;
};

} // namespace am
