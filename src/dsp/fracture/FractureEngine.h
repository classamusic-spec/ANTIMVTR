#pragma once

#include "dsp/RenderContext.h"

namespace am
{

/**
    FRACTURE — spectral fragmentation applied after the voices are mixed.
    Phase 0: bypass with correct latency reporting (0).
*/
class FractureEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (float* l, float* r, int n, const RenderContext& ctx);
    int  latencySamples() const noexcept { return 0; }
    float activity() const noexcept { return lastActivity; }
    int fftSize() const noexcept { return 0; }
    int hopSize() const noexcept { return 0; }

    /** Message thread: frees data handed off to the audio thread (see core/RealtimeHandoff.h). */
    void messageThreadMaintenance() {}

private:
    double sr = 48000.0;
    float lastActivity = 0.0f;
};

} // namespace am
