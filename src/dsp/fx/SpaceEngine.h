#pragma once

#include "dsp/RenderContext.h"

namespace am
{

/**
    SPACE — curated macro FX environments over an internal rack.
    Phase 0: bypass.
*/
class SpaceEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (float* l, float* r, int n, const RenderContext& ctx);
    float activity() const noexcept { return lastActivity; }
    int latencySamples() const noexcept { return 0; }

    /** Message thread: frees data handed off to the audio thread (see core/RealtimeHandoff.h). */
    void messageThreadMaintenance() {}

private:
    double sr = 48000.0;
    float lastActivity = 0.0f;
};

} // namespace am
