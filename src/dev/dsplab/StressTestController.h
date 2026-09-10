#pragma once

#include "LabWidgets.h"
#include "dev/diagnostics/SignalMetrics.h"
#include "dev/diagnostics/StressTestGenerator.h"

namespace am::dev
{

/**
    STRESS TESTS (§84)

    Drives the ten stress scenarios from the message thread: a timer asks
    StressTestGenerator what is due, and the actions are injected as MIDI
    through AntiMatrProcessor::injectMidi or written to the host parameter
    tree — the audio thread is never touched directly.

    While a scenario runs the controller samples the diagnostic snapshot and
    the master tap for peak, non-finite samples, CPU and safety events, and
    turns them into a verdict.
*/
class StressTestController : public juce::Component,
                             private juce::Timer
{
public:
    explicit StressTestController (AntiMatrProcessor& processor);
    ~StressTestController() override;

    void start (StressTest test);
    void stop();
    bool isRunning() const noexcept { return generator.isRunning(); }

    /** Called by the shell every refresh so the verdict follows the engine. */
    void sample (const LabFrame& f);

    void resized() override;

private:
    void timerCallback() override;
    void execute (const StressAction& a);
    void refreshStatus();

    struct Verdict
    {
        float    maxPeak = 0.0f;
        float    maxCpu = 0.0f;
        int      nonFinite = 0;
        uint32_t safetyAtStart = 0;
        uint32_t safetyNow = 0;
        uint32_t overrunsAtStart = 0;
        uint32_t overrunsNow = 0;
        int      maxVoices = 0;
        int      maxNodes = 0;
        int      samples = 0;
    };

    AntiMatrProcessor& processor;
    StressTestGenerator generator;
    Verdict verdict;
    juce::String verdictText { "idle" };

    juce::ComboBox testBox;
    juce::TextButton startButton { "START" }, stopButton { "STOP" };
    LabPanel panel { "Stress tests  (\u00a784)" };
    KeyValueTable runInfo, measured;

    double startTimeMs = 0.0;
    std::vector<StressAction> pending;
    std::array<float, 2048> tapL {}, tapR {};
};

} // namespace am::dev
