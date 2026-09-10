#pragma once

#include "EngineEventLogView.h"
#include "EvolveView.h"
#include "ExcitationView.h"
#include "LabWidgets.h"
#include "MatterCapture.h"
#include "MatterInspector.h"
#include "ParameterSweepTool.h"
#include "ParameterTraceView.h"
#include "PerformanceView.h"
#include "PresetValidatorView.h"
#include "SafetyView.h"
#include "SignalInspector.h"
#include "StageComparisonView.h"
#include "StressTestController.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

/**
    DSP LAB — internal engineering workspace (§68–90).

    Observes the production engine purely through the Diagnostics interfaces
    (triple-buffered snapshots, audio taps, safety counters, the lock-free
    event queue) and writes only into DevControls and the host parameter
    tree. Nothing in src/dsp depends on anything here.

    Layout (§70): LEFT engine inspector, CENTER the selected tab, RIGHT
    profiling / diagnostics, BOTTOM the A/B, dry-mode and stress tools plus a
    test keyboard. One 25 Hz timer reads a single snapshot per frame and
    refreshes only the views that are on screen, so hidden tabs cost nothing.
*/
class DSPLabView : public juce::Component,
                   private juce::Timer
{
public:
    explicit DSPLabView (AntiMatrProcessor& p);
    ~DSPLabView() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    /** Builds the diagnostic report (§88) — no user audio is included. */
    juce::String buildReport() const;

    /** Selects a DSP LAB tab (also used by the snapshot tool). */
    void selectTab (int index);
    int  numTabs() const noexcept { return tabs.getNumTabs(); }

    /** Starts one of the §84 stress scenarios (index into StressTest). */
    void startStressTest (int index);
    /** Starts a §85 parameter sweep (-1 = the parameter already selected). */
    void startParameterSweep (int parameter = -1);
    /** Starts the §86 factory preset validation on its background thread. */
    void startPresetValidation();

    /** Tab order. OVERVIEW..SAFETY match the §70 list; the tools follow. */
    enum Tab
    {
        Overview = 0, Source, Matter, Evolve, Fracture, Mod, Space,
        Performance, Safety, Sweep, Presets, Events, NumTabs
    };

private:
    void timerCallback() override;
    void drainEvents();
    void refreshEngineInspector (const DiagnosticSnapshot& s);
    void refreshProfilingPanel (const DiagnosticSnapshot& s);
    void exportReport (bool chooseFile);
    void exportReport (const juce::File& file);
    LabView* viewForTab (int index) const;
    /** Headless verification hook: ANTIMATR_LAB_AUTORUN=stress:<n>|sweep[:id]|presets
        starts a tool on construction so the snapshot tool can capture a run in
        progress. Developer builds only; it never runs unless the variable is set. */
    void applyAutoRunFromEnvironment();

    juce::File autoRunReportFile;   ///< written once the snapshot has real content

    AntiMatrProcessor& processor;
    DiagnosticSnapshot snapshot;
    MatterCaptureStore captures;
    int frameCounter = 0;

    juce::TabbedButtonBar tabs { juce::TabbedButtonBar::TabsAtTop };
    int currentTab = Overview;

    // LEFT — engine inspector
    KeyValueTable engineInspector { "Engine inspector" };
    StageMeters stageMeters;

    // RIGHT — profiling / diagnostics
    KeyValueTable profilingPanel { "CPU  moving / avg / peak %" };
    KeyValueTable safetyPanel { "Safety counters" };

    // CENTER — one view per tab
    SignalInspector overview;
    ExcitationView sourceView;
    MatterInspector matterView;
    EvolveView evolveView;
    FractureView fractureView;
    ParameterTraceView modView;
    SpaceView spaceView;
    PerformanceView performanceView;
    SafetyView safetyView;
    ParameterSweepTool sweepView;
    PresetValidatorView presetView;
    EngineEventLogView eventView;

    // BOTTOM — A/B, dev controls, stress tools, keyboard
    LabPanel devPanel { "Dev controls" };
    juce::ComboBox focusBox;
    juce::ToggleButton bypassEvolve { "Bypass Evolve" }, bypassFracture { "Bypass Fracture" },
                       bypassSpace { "Bypass Space" }, profilingToggle { "Profiling" };
    LabPanel abPanel { "A / B" };
    juce::TextButton slotA { "A" }, slotB { "B" }, copyAB { "COPY ->" },
                     exportButton { "EXPORT REPORT" }, exportAsButton { "EXPORT AS..." };
    StressTestController stress;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSPLabView)
};

} // namespace am::dev
