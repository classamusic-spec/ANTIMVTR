#pragma once

#include "plugin/AntiMatrProcessor.h"
#include "ui/AntiMatrTheme.h"
#include "ui/visualizers/SpectrumAnalyzer.h"

namespace am::dev
{

/**
    DSP LAB — internal engineering workspace.

    Observes the production engine purely through the Diagnostics interfaces
    (snapshots, taps, counters, event queue) and writes only to DevControls.
    Phase 0 shell: engine inspector, signal inspector (waveform / spectrum per
    stage), Matter node list + frequency distribution, per-subsystem CPU,
    safety counters, event log, dry-mode / bypass / focus controls and a test
    keyboard. Later phases add the dedicated tabs described in the spec.
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

    /** Builds the diagnostic report (plain JSON) — no user audio is included. */
    juce::String buildReport() const;

    /** Selects a DSP LAB tab (also used by the snapshot tool). */
    void selectTab (int index);
    int  numTabs() const noexcept { return tabs.getNumTabs(); }

private:
    class SignalInspector;
    class NodeInspector;
    class TableView;

    void timerCallback() override;
    void refreshTables();

    AntiMatrProcessor& processor;
    DiagnosticSnapshot snapshot;

    juce::TabbedButtonBar tabs { juce::TabbedButtonBar::TabsAtTop };
    std::unique_ptr<TableView> engineTable, perfTable, safetyTable, controlTable;
    std::unique_ptr<SignalInspector> signal;
    std::unique_ptr<NodeInspector> nodes;

    juce::ComboBox stageBox, dryModeBox, focusBox;
    juce::ToggleButton bypassEvolve { "Bypass Evolve" }, bypassFracture { "Bypass Fracture" }, bypassSpace { "Bypass Space" }, profiling { "Profiling" };
    juce::TextButton resetSafety { "Reset safety" }, exportReport { "Export report" }, clearLog { "Clear log" };
    juce::TextEditor eventLog;
    juce::MidiKeyboardComponent keyboard;

    int currentTab = 0;
    int logLines = 0;
};

} // namespace am::dev
