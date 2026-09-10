#include "DSPLabView.h"

#include "dev/diagnostics/DiagnosticReport.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    const char* kTabNames[] =
    {
        "OVERVIEW", "SOURCE", "MATTER", "EVOLVE", "FRACTURE", "MOD", "SPACE",
        "PERFORMANCE", "SAFETY", "SWEEP", "PRESETS", "EVENTS"
    };

    const char* qualityName (uint8_t q) noexcept
    {
        static const char* names[] = { "ECO", "NORMAL", "HIGH", "ULTRA" };
        return names[juce::jlimit (0, 3, (int) q)];
    }

    const char* dryModeName (uint8_t d) noexcept
    {
        static const char* names[] = { "FULL SYNTH", "SOURCE ONLY", "MATTER ONLY", "MATTER + EVOLVE" };
        return names[juce::jlimit (0, 3, (int) d)];
    }

    juce::Colour loadColour (float percent) noexcept
    {
        if (percent >= 90.0f) return Theme::magenta;
        if (percent >= 60.0f) return Theme::amber;
        return Theme::textPrimary;
    }
}

//==============================================================================
DSPLabView::DSPLabView (AntiMatrProcessor& p)
    : processor (p),
      matterView (captures),
      evolveView (captures),
      sweepView (p),
      stress (p),
      keyboard (p.keyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    static_assert (sizeof (kTabNames) / sizeof (kTabNames[0]) == (size_t) NumTabs,
                   "tab name table must match the Tab enum");

    for (auto* name : kTabNames)
        tabs.addTab (name, Theme::panelTop, -1);
    tabs.setColour (juce::TabbedButtonBar::tabTextColourId, Theme::textSecondary);
    tabs.setColour (juce::TabbedButtonBar::frontTextColourId, Theme::cyan);
    tabs.setColour (juce::TabbedButtonBar::tabOutlineColourId, Theme::borderSoft);
    tabs.setColour (juce::TabbedButtonBar::frontOutlineColourId, Theme::cyan.withAlpha (0.4f));
    addAndMakeVisible (tabs);

    // ---- left / right persistent panels
    engineInspector.setRowHeight (14.0f);
    engineInspector.setLabelWidthFraction (0.56f);
    addAndMakeVisible (engineInspector);
    addAndMakeVisible (stageMeters);

    profilingPanel.setRowHeight (14.0f);
    profilingPanel.setLabelWidthFraction (0.46f);
    profilingPanel.setAccent (Theme::amber);
    addAndMakeVisible (profilingPanel);

    safetyPanel.setRowHeight (14.0f);
    safetyPanel.setLabelWidthFraction (0.62f);
    safetyPanel.setAccent (Theme::magenta);
    addAndMakeVisible (safetyPanel);

    // ---- centre views
    for (int i = 0; i < NumTabs; ++i)
        if (auto* view = viewForTab (i))
            addChildComponent (*view);

    overview.setStage (Stage::Master);

    // ---- bottom: dev controls
    focusBox.addItem ("FOCUS: LATEST VOICE", 1);
    for (int i = 0; i < kMaxVoices; ++i)
        focusBox.addItem ("FOCUS: VOICE " + juce::String (i), i + 2);
    focusBox.setSelectedId (1, juce::dontSendNotification);
    focusBox.onChange = [this]
    {
        processor.diagnostics().dev.focusVoice.store (focusBox.getSelectedId() - 2, std::memory_order_relaxed);
    };
    styleCombo (focusBox);
    devPanel.addAndMakeVisible (focusBox);

    auto& dev = processor.diagnostics().dev;
    styleToggle (bypassEvolve, Theme::violet);
    styleToggle (bypassFracture, Theme::magenta);
    styleToggle (bypassSpace, Theme::ivory);
    styleToggle (profilingToggle, Theme::amber);
    bypassEvolve.onClick   = [this, &dev] { dev.bypassEvolve.store (bypassEvolve.getToggleState(), std::memory_order_relaxed); };
    bypassFracture.onClick = [this, &dev] { dev.bypassFracture.store (bypassFracture.getToggleState(), std::memory_order_relaxed); };
    bypassSpace.onClick    = [this, &dev] { dev.bypassSpace.store (bypassSpace.getToggleState(), std::memory_order_relaxed); };
    profilingToggle.setToggleState (dev.profiling.load(), juce::dontSendNotification);
    profilingToggle.onClick = [this, &dev] { dev.profiling.store (profilingToggle.getToggleState(), std::memory_order_relaxed); };
    for (auto* t : { &bypassEvolve, &bypassFracture, &bypassSpace, &profilingToggle })
        devPanel.addAndMakeVisible (t);
    addAndMakeVisible (devPanel);

    // ---- bottom: A/B + report export
    styleButton (slotA, Theme::cyan);
    styleButton (slotB, Theme::violet);
    styleButton (copyAB);
    styleButton (exportButton, Theme::amber);
    styleButton (exportAsButton, Theme::amber);
    slotA.onClick = [this] { processor.selectABSlot (0); };
    slotB.onClick = [this] { processor.selectABSlot (1); };
    copyAB.onClick = [this] { processor.copyABToOther(); };
    exportButton.onClick = [this] { exportReport (false); };
    exportAsButton.onClick = [this] { exportReport (true); };
    for (auto* b : { &slotA, &slotB, &copyAB, &exportButton, &exportAsButton })
        abPanel.addAndMakeVisible (b);
    addAndMakeVisible (abPanel);

    addAndMakeVisible (stress);

    keyboard.setAvailableRange (24, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, Theme::textSecondary);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, Theme::background);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Theme::border);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::cyan.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::cyan.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, Theme::background);
    addAndMakeVisible (keyboard);

    selectTab (Overview);
}

DSPLabView::~DSPLabView()
{
    stopTimer();
}

LabView* DSPLabView::viewForTab (int index) const
{
    auto* self = const_cast<DSPLabView*> (this);
    switch (index)
    {
        case Overview:    return &self->overview;
        case Source:      return &self->sourceView;
        case Matter:      return &self->matterView;
        case Evolve:      return &self->evolveView;
        case Fracture:    return &self->fractureView;
        case Mod:         return &self->modView;
        case Space:       return &self->spaceView;
        case Performance: return &self->performanceView;
        case Safety:      return &self->safetyView;
        case Sweep:       return &self->sweepView;
        case Presets:     return &self->presetView;
        case Events:      return &self->eventView;
        default:          return nullptr;
    }
}

void DSPLabView::selectTab (int index)
{
    index = juce::jlimit (0, juce::jmax (0, tabs.getNumTabs() - 1), index);
    if (tabs.getCurrentTabIndex() != index)
        tabs.setCurrentTabIndex (index, juce::dontSendNotification);
    currentTab = index;

    for (int i = 0; i < NumTabs; ++i)
        if (auto* view = viewForTab (i))
            view->setVisible (i == index);

    resized();

    // Give the new tab a frame immediately so a snapshot right after a tab
    // switch is never empty.
    if (auto* view = viewForTab (index))
    {
        const LabFrame frame { processor, processor.diagnostics(), snapshot,
                               processor.engine().sampleRate(), frameCounter };
        view->updateFrame (frame);
    }
}

void DSPLabView::visibilityChanged()
{
    if (isShowing())
    {
        if (! isTimerRunning()) startTimerHz (25);
    }
    else
    {
        stopTimer();
    }
}

void DSPLabView::parentHierarchyChanged()
{
    if (isShowing() && ! isTimerRunning())
        startTimerHz (25);
}

//==============================================================================
void DSPLabView::timerCallback()
{
    if (! isShowing())
        return;

    if (tabs.getCurrentTabIndex() != currentTab)
        selectTab (tabs.getCurrentTabIndex());

    processor.diagnostics().diagnosticSnapshots.read (snapshot);
    ++frameCounter;

    const LabFrame frame { processor, processor.diagnostics(), snapshot,
                           processor.engine().sampleRate(), frameCounter };

    refreshEngineInspector (snapshot);
    refreshProfilingPanel (snapshot);
    stageMeters.setLevels (snapshot);

    drainEvents();

    // Only the visible tab does work.
    if (auto* view = viewForTab (currentTab))
        view->updateFrame (frame);

    stress.sample (frame);

    const int slot = processor.currentABSlot();
    slotA.setToggleState (slot == 0, juce::dontSendNotification);
    slotB.setToggleState (slot == 1, juce::dontSendNotification);

    auto& dev = processor.diagnostics().dev;
    bypassEvolve.setToggleState (dev.bypassEvolve.load (std::memory_order_relaxed), juce::dontSendNotification);
    bypassFracture.setToggleState (dev.bypassFracture.load (std::memory_order_relaxed), juce::dontSendNotification);
    bypassSpace.setToggleState (dev.bypassSpace.load (std::memory_order_relaxed), juce::dontSendNotification);
    profilingToggle.setToggleState (dev.profiling.load (std::memory_order_relaxed), juce::dontSendNotification);
}

void DSPLabView::drainEvents()
{
    // Single drain point: the queue is single-consumer, so the shell reads it
    // and hands each event to the views that log them.
    const auto sampleRate = (uint64_t) juce::jmax (1.0, processor.engine().sampleRate());
    processor.diagnostics().events.drain ([this, sampleRate] (const EngineEvent& e)
    {
        eventView.addEvent (e, sampleRate);
        if (e.type == EngineEventType::SafetyEvent || e.type == EngineEventType::SafetyReset)
            safetyView.addEvent (e);
    });
}

void DSPLabView::refreshEngineInspector (const DiagnosticSnapshot& s)
{
    engineInspector.setRows ({
        { "Sample rate",     juce::String (s.sampleRate, 0) + " Hz" },
        { "Block size",      juce::String (s.blockSize) + " smp" },
        { "Quality",         qualityName (s.quality) },
        { "Dry mode",        dryModeName (s.dryMode) },
        { "Voices",          juce::String (s.activeVoices) + " / " + juce::String (s.maxVoices) },
        { "Latency",         juce::String (s.latencySamples) + " smp" },
        { "Sample clock",    juce::String (s.sampleTime) },
        { "MATTER", "" },
        { "Focus voice",     juce::String (s.focusVoice) },
        { "Focus note",      juce::String (s.focusNote) },
        { "Fundamental",     juce::String (s.fundamentalHz, 1) + " Hz" },
        { "Nodes",           juce::String (s.activeNodes) + " / " + juce::String (s.numNodes) },
        { "Edges",           juce::String (s.numEdges) },
        { "Clusters",        juce::String (s.clusterCount) },
        { "Topology seed",   juce::String (s.topologySeed) },
        { "Energy",          juce::String (s.matterEnergy, 5) },
        { "Coupling avg/max",juce::String (s.averageCoupling, 3) + " / " + juce::String (s.maxCoupling, 3) },
        { "Material A/B",    juce::String ((int) s.materialA) + " / " + juce::String ((int) s.materialB)
                             + "  " + juce::String (s.materialBlend, 2) },
        { "FRACTURE", "" },
        { "FFT / hop",       juce::String (s.fractureFFTSize) + " / " + juce::String (s.fractureHop) },
        { "Activity",        juce::String (s.fractureActivity, 4) },
        { "EVENTS", "" },
        { "Dropped",         juce::String (s.eventsDropped) },
        { "Preset",          processor.currentPresetName() },
    });

    engineInspector.clearRowColours();
    if (s.dryMode != (uint8_t) DryMode::FullSynth)
        engineInspector.setRowColour (3, Theme::amber);
    if (s.eventsDropped > 0)
        engineInspector.setRowColour (22, Theme::amber);
}

void DSPLabView::refreshProfilingPanel (const DiagnosticSnapshot& s)
{
    std::vector<KeyValueTable::Row> rows;
    rows.reserve ((size_t) PerformanceProfiler::kNumSubsystems + 8);

    for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i)
        rows.push_back ({ juce::String (subsystemName ((Subsystem) i)),
                          juce::String (s.perf.movingPercent[i], 2) + " / "
                        + juce::String (s.perf.avgPercent[i], 2) + " / "
                        + juce::String (s.perf.peakPercent[i], 2) });

    rows.push_back ({ "TOTAL", juce::String (s.perf.totalMovingPercent, 2) + " / "
                             + juce::String (s.perf.totalAvgPercent, 2) + " / "
                             + juce::String (s.perf.totalPeakPercent, 2) });
    rows.push_back ({ "Block / budget", juce::String (s.perf.lastBlockMicros, 1) + " / "
                                      + juce::String (s.perf.budgetMicros, 1) + " us" });
    rows.push_back ({ "Overruns", juce::String (s.perf.overruns) + " / " + juce::String (s.perf.blocksMeasured) });

    profilingPanel.setRows (std::move (rows));
    profilingPanel.clearRowColours();
    profilingPanel.setRowColour (PerformanceProfiler::kNumSubsystems, loadColour (s.perf.totalMovingPercent));
    if (s.perf.overruns > 0)
        profilingPanel.setRowColour (PerformanceProfiler::kNumSubsystems + 2, Theme::magenta);

    std::vector<KeyValueTable::Row> safetyRows;
    std::vector<int> hot;
    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
    {
        juce::String value (s.safety.counts[i]);
        if (s.safety.counts[i] > 0)
        {
            value << "  " << subsystemName ((Subsystem) s.safety.lastSubsystem[i]);
            if (s.safety.lastVoice[i] >= 0)
                value << " v" << juce::String ((int) s.safety.lastVoice[i]);
            hot.push_back (i);
        }
        safetyRows.push_back ({ juce::String (SafetyMonitor::eventName ((SafetyEvent) i)), value });
    }
    safetyRows.push_back ({ "TOTAL", juce::String ((int) s.safety.total) });

    safetyPanel.setRows (std::move (safetyRows));
    safetyPanel.clearRowColours();
    for (int i : hot)
    {
        const auto event = (SafetyEvent) i;
        const bool critical = event == SafetyEvent::NaN || event == SafetyEvent::Infinity
                           || event == SafetyEvent::HardClip || event == SafetyEvent::InvalidCoefficient
                           || event == SafetyEvent::InvalidFrequency;
        safetyPanel.setRowColour (i, critical ? Theme::magenta : Theme::amber);
    }
    safetyPanel.setRowColour (SafetyMonitor::kNumEvents, s.safety.total > 0 ? Theme::amber : Theme::textDim);
    safetyPanel.setTitle (s.safety.total > 0 ? "Safety counters  *" : "Safety counters");
}

//==============================================================================
juce::String DSPLabView::buildReport() const
{
    auto in = DiagnosticReportInput::environment();
    in.presetName = processor.currentPresetName();
    in.presetTags = processor.currentPresetTags();
    in.abSlot = processor.currentABSlot();
    in.snapshot = snapshot;
    in.parameters = processor.currentParamValues();
    return DiagnosticReport::toJson (in);
}

void DSPLabView::exportReport (bool chooseFile)
{
    auto write = [this] (const juce::File& file)
    {
        auto in = DiagnosticReportInput::environment();
        in.presetName = processor.currentPresetName();
        in.presetTags = processor.currentPresetTags();
        in.abSlot = processor.currentABSlot();
        in.snapshot = snapshot;
        in.parameters = processor.currentParamValues();

        const auto result = DiagnosticReport::writeTo (file, in);
        EngineEvent e;
        e.type = EngineEventType::Custom;
        e.subsystem = (uint8_t) Subsystem::State;
        e.sampleTime = snapshot.sampleTime;
        eventView.addEvent (e, (uint64_t) juce::jmax (1.0, snapshot.sampleRate));
        exportButton.setButtonText (result.wasOk() ? "REPORT SAVED" : "EXPORT FAILED");
    };

    if (! chooseFile)
    {
        write (DiagnosticReport::defaultFile());
        return;
    }

    chooser = std::make_unique<juce::FileChooser> ("Export DSP LAB diagnostic report",
                                                   DiagnosticReport::defaultFile(), "*.json");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                          [write] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file != juce::File())
                                  write (file);
                          });
}

//==============================================================================
void DSPLabView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    auto header = getLocalBounds().toFloat().removeFromTop (24.0f).reduced (10.0f, 2.0f);
    draw::trackedText (g, "DSP LAB", header.removeFromLeft (90.0f), juce::Justification::centredLeft,
                       Theme::titleFont (12.0f), Theme::amber);
    draw::trackedText (g, "INTERNAL ENGINEERING WORKSPACE - OBSERVES THE ENGINE THROUGH DIAGNOSTICS ONLY",
                       header.removeFromLeft (520.0f), juce::Justification::centredLeft,
                       Theme::captionFont (8.5f), Theme::textDim);

    const juce::String right = juce::String (ANTIMATR_VERSION_STRING) + "   "
                             + juce::String (snapshot.sampleRate / 1000.0, 1) + " kHz / "
                             + juce::String (snapshot.blockSize) + "   "
                             + juce::String (snapshot.activeVoices) + " voices";
    draw::trackedText (g, right, header, juce::Justification::centredRight, Theme::captionFont (8.5f), Theme::textSecondary);
}

void DSPLabView::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (18);
    tabs.setBounds (area.removeFromTop (26));
    area.removeFromTop (6);

    // BOTTOM — keyboard, then the dev / A-B / stress strip.
    keyboard.setBounds (area.removeFromBottom (54));
    area.removeFromBottom (6);
    auto bottom = area.removeFromBottom (juce::jmax (150, area.getHeight() / 5));
    area.removeFromBottom (6);

    {
        auto strip = bottom;
        auto devArea = strip.removeFromLeft (juce::jmax (250, strip.getWidth() * 22 / 100));
        strip.removeFromLeft (6);
        auto abArea = strip.removeFromLeft (juce::jmax (190, strip.getWidth() * 24 / 100));
        strip.removeFromLeft (6);
        stress.setBounds (strip);

        devPanel.setBounds (devArea);
        {
            auto inner = devPanel.contentBounds();
            focusBox.setBounds (inner.removeFromTop (22));
            inner.removeFromTop (5);
            auto row = inner.removeFromTop (22);
            bypassEvolve.setBounds (row.removeFromLeft (row.getWidth() / 2));
            bypassFracture.setBounds (row);
            row = inner.removeFromTop (22);
            bypassSpace.setBounds (row.removeFromLeft (row.getWidth() / 2));
            profilingToggle.setBounds (row);
        }

        abPanel.setBounds (abArea);
        {
            auto inner = abPanel.contentBounds();
            auto row = inner.removeFromTop (22);
            slotA.setBounds (row.removeFromLeft (row.getWidth() / 3).reduced (1, 0));
            slotB.setBounds (row.removeFromLeft (row.getWidth() / 2).reduced (1, 0));
            copyAB.setBounds (row.reduced (1, 0));
            inner.removeFromTop (5);
            exportButton.setBounds (inner.removeFromTop (22).reduced (1, 0));
            inner.removeFromTop (4);
            exportAsButton.setBounds (inner.removeFromTop (22).reduced (1, 0));
        }
    }

    // LEFT — engine inspector.
    auto left = area.removeFromLeft (juce::jmax (216, area.getWidth() * 15 / 100));
    area.removeFromLeft (6);
    stageMeters.setBounds (left.removeFromBottom (juce::jmax (130, left.getHeight() * 26 / 100)));
    left.removeFromBottom (6);
    engineInspector.setBounds (left);

    // RIGHT — profiling / diagnostics.
    auto right = area.removeFromRight (juce::jmax (232, area.getWidth() * 18 / 100));
    area.removeFromRight (6);
    profilingPanel.setBounds (right.removeFromTop (juce::jmax (220, right.getHeight() * 52 / 100)));
    right.removeFromTop (6);
    safetyPanel.setBounds (right);

    // CENTER — the selected tab.
    for (int i = 0; i < NumTabs; ++i)
        if (auto* view = viewForTab (i))
            view->setBounds (area);
}

} // namespace am::dev
