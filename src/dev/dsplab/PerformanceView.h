#pragma once

#include "LabWidgets.h"

namespace am::dev
{

/** Rolling history of the total block cost, sampled at the shell's timer rate. */
class CpuHistoryView : public LabPanel
{
public:
    CpuHistoryView();

    void push (float movingPercent, float peakPercent);
    void paint (juce::Graphics& g) override;

    static constexpr int kHistory = 300;

private:
    std::array<float, kHistory> moving {}, peak {};
    int write = 0;
    bool filled = false;
};

//==============================================================================
/**
    PERFORMANCE PROFILER (§78–79)

    Per-subsystem average / peak / moving percentage of the block budget, the
    block budget itself, overruns, voice count, sample rate, buffer size and
    quality, a rolling CPU history graph and the memory diagnostics.

    Memory is reported as sizeof-based estimates of the preallocated engine
    structures plus the events-dropped counter. Counting allocations inside
    the audio callback needs an engine hook DSP LAB cannot add (see the
    panel's own note).
*/
class PerformanceView : public LabView
{
public:
    PerformanceView();

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void rebuildSubsystems (const DiagnosticSnapshot& s);
    void rebuildEngine (const LabFrame& f);
    void rebuildMemory (const LabFrame& f);

    LabPanel tablePanel { "CPU per subsystem  (% of block budget)" };
    LabTable subsystems;
    CpuHistoryView history;
    KeyValueTable engineInfo { "Engine" };
    KeyValueTable memory { "Memory diagnostics" };
    juce::ToggleButton profiling { "Profiling enabled" };
    juce::TextButton resetProfiler { "RESET PROFILER" };
    LabPanel controls { "Profiler" };
    Diagnostics* diagnostics = nullptr;
};

} // namespace am::dev
