#pragma once

#include "DiagnosticSnapshot.h"
#include "core/LockFreeQueue.h"
#include "core/RealtimeUtils.h"

namespace am
{

/**
    Aggregates every diagnostic facility the engine exposes.

    The production engine writes into this object from the audio thread using
    only wait-free operations. DSP LAB and the consumer UI read from it on
    the message thread. Nothing here is required for audio to work.
*/
struct Diagnostics
{
    static constexpr int kTapSize = 8192;

    Diagnostics()
    {
        safety.attachEventQueue (&events);
        safety.setSampleClock (&sampleClock);
    }

    SafetyMonitor       safety;
    PerformanceProfiler profiler;
    EngineEventQueue    events;
    DevControls         dev;

    TripleBuffer<DiagnosticSnapshot>  diagnosticSnapshots;
    TripleBuffer<VisualStateSnapshot> visualSnapshots;

    std::array<AudioTapRing<kTapSize>, (int) Stage::Count> taps;

    std::atomic<uint64_t> sampleClock { 0 };

    void clearTaps() noexcept { for (auto& t : taps) t.clear(); }
};

} // namespace am
