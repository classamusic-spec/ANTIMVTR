#pragma once

#include "LabWidgets.h"
#include "dev/diagnostics/SignalMetrics.h"

namespace am::dev
{

/** Plot of a completed or running sweep: peak, RMS, CPU and centroid against
    the swept parameter, with the dangerous steps shaded. */
class SweepPlotView : public LabPanel
{
public:
    SweepPlotView();
    void setPoints (const std::vector<SweepPoint>* p) { points = p; repaint(); }
    void paint (juce::Graphics& g) override;

private:
    const std::vector<SweepPoint>* points = nullptr;
};

//==============================================================================
/**
    PARAMETER SWEEP TOOL (§85)

    Sweeps one parameter from 0 to 1 over N seconds while a test note plays
    and measures, per step: peak, RMS (Master tap), CPU, non-finite samples,
    spectral centroid (juce::dsp::FFT), crest factor, active Matter nodes and
    the safety events the step raised — then flags the dangerous zones.

    Everything runs on the message thread: the parameter is written through
    the host tree, the measurements come from the diagnostics taps.
*/
class ParameterSweepTool : public LabView,
                           private juce::Timer
{
public:
    explicit ParameterSweepTool (AntiMatrProcessor& processor);
    ~ParameterSweepTool() override;

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    void startSweep();
    void stopSweep (bool restoreParameter);
    void timerCallback() override;
    void measureCurrentStep();
    void rebuildTable();
    void refreshSummary();
    void populateParameterList();

    AntiMatrProcessor& processor;

    juce::ComboBox parameterBox;
    juce::Slider seconds, steps, noteSlider;
    juce::TextButton startButton { "START SWEEP" }, stopButton { "STOP" };
    LabPanel controls { "Sweep  (§85)" };
    juce::Label secondsLabel, stepsLabel, noteLabel;

    LabPanel tablePanel { "Sweep measurements" };
    LabTable table;
    SweepPlotView plotView;
    KeyValueTable summary { "Sweep summary" };

    std::vector<SweepPoint> points;
    SweepSummary summaryData;
    SpectrumMeasurement spectrum { 11 };

    bool running = false;
    int currentStep = 0;
    int totalSteps = 33;
    double dwellSeconds = 0.15;
    double stepStartMs = 0.0;
    int sweptParameter = -1;
    float originalNormalised = 0.0f;
    int testNote = 60;
    uint32_t safetyAtStepStart = 0;

    const DiagnosticSnapshot* latest = nullptr;
    Diagnostics* diagnostics = nullptr;
    double sampleRate = 48000.0;
    std::array<float, 4096> tapL {}, tapR {}, tapMono {};
};

} // namespace am::dev
