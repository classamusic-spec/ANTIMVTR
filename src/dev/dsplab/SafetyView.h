#pragma once

#include "LabWidgets.h"

namespace am::dev
{

/**
    SAFETY MONITOR (§80)

    Every SafetyEvent counter with the subsystem and voice of its last
    occurrence, a reset, and an auto-refreshing log of the safety events the
    engine pushed through the lock-free queue. Non-zero counters are lifted
    into amber, and the categories that mean the audio was actually damaged
    (NaN, infinity, hard clip) into magenta.
*/
class SafetyView : public LabView
{
public:
    SafetyView();

    /** Called by the shell for every safety event drained from the queue. */
    void addEvent (const EngineEvent& e);

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void rebuildCounters (const DiagnosticSnapshot& s);

    LabPanel tablePanel { "Safety counters" };
    LabTable counters;
    KeyValueTable summary { "Summary" };
    LabPanel logPanel { "Safety log  (first occurrences per category)" };
    juce::TextEditor log;
    juce::TextButton resetCounters { "RESET COUNTERS" }, clearLog { "CLEAR LOG" };
    LabPanel controls { "Controls" };

    Diagnostics* diagnostics = nullptr;
    uint32_t lastTotal = 0;
    int logLines = 0;
};

} // namespace am::dev
