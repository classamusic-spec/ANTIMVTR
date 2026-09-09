#include "DSPLabView.h"
#include "ui/components/AMDrawing.h"
#include "state/StateManager.h"

namespace am::dev
{

using namespace am::ui;

//==============================================================================
/** Simple two-column key/value table used throughout DSP LAB. */
class DSPLabView::TableView : public juce::Component
{
public:
    explicit TableView (const juce::String& t) : title (t) {}

    void set (std::vector<std::pair<juce::String, juce::String>> newRows)
    {
        rows = std::move (newRows);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        draw::insetSurface (g, b, 6.0f);
        auto area = b.reduced (8.0f, 6.0f);
        draw::trackedText (g, title.toUpperCase(), area.removeFromTop (16.0f), juce::Justification::centredLeft, Theme::captionFont (9.0f), Theme::cyan);
        g.setFont (Theme::font (11.0f));
        const float rowH = 15.0f;
        for (const auto& r : rows)
        {
            if (area.getHeight() < rowH) break;
            auto row = area.removeFromTop (rowH);
            g.setColour (Theme::textSecondary);
            g.drawText (r.first, row.removeFromLeft (row.getWidth() * 0.55f), juce::Justification::centredLeft, false);
            g.setColour (Theme::textPrimary);
            g.drawText (r.second, row, juce::Justification::centredRight, false);
        }
    }

private:
    juce::String title;
    std::vector<std::pair<juce::String, juce::String>> rows;
};

//==============================================================================
/** Waveform + spectrum of a selectable engine stage. Technical style, log-frequency spectrum. */
class DSPLabView::SignalInspector : public juce::Component
{
public:
    explicit SignalInspector (Diagnostics& d) : diag (d) {}

    void setStage (Stage s) { stage = s; }

    void refresh (double sampleRate)
    {
        diag.taps[(int) stage].readLatest (waveL.data(), waveR.data(), (int) waveL.size());
        for (size_t i = 0; i < mono.size(); ++i) mono[i] = 0.5f * (waveL[i] + waveR[i]);
        analyzer.compute (mono.data(), sampleRate, bands.data(), (int) bands.size(), -96.0f);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto top = b.removeFromTop (b.getHeight() * 0.45f).reduced (0.0f, 3.0f);
        auto bottom = b.reduced (0.0f, 3.0f);

        draw::insetSurface (g, top, 6.0f);
        draw::insetSurface (g, bottom, 6.0f);

        // Waveform (last 2048 samples), both channels.
        {
            auto inner = top.reduced (6.0f);
            draw::trackedText (g, "WAVEFORM  " + juce::String (stageName (stage)).toUpperCase(), inner.removeFromTop (14.0f), juce::Justification::centredLeft, Theme::captionFont (9.0f), Theme::textSecondary);
            g.setColour (Theme::borderSoft);
            g.drawLine (inner.getX(), inner.getCentreY(), inner.getRight(), inner.getCentreY(), 1.0f);
            for (float ref : { 0.5f, 1.0f })
            {
                g.setColour (juce::Colours::white.withAlpha (0.04f));
                g.drawLine (inner.getX(), inner.getCentreY() - ref * inner.getHeight() * 0.5f, inner.getRight(), inner.getCentreY() - ref * inner.getHeight() * 0.5f, 1.0f);
                g.drawLine (inner.getX(), inner.getCentreY() + ref * inner.getHeight() * 0.5f, inner.getRight(), inner.getCentreY() + ref * inner.getHeight() * 0.5f, 1.0f);
            }
            auto drawChannel = [&] (const std::array<float, 2048>& data, juce::Colour c)
            {
                juce::Path p;
                const int n = (int) data.size();
                for (int i = 0; i < n; ++i)
                {
                    const float x = inner.getX() + inner.getWidth() * (float) i / (float) (n - 1);
                    const float y = inner.getCentreY() - juce::jlimit (-1.0f, 1.0f, data[(size_t) i]) * inner.getHeight() * 0.5f;
                    if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                }
                g.setColour (c);
                g.strokePath (p, juce::PathStrokeType (1.0f));
            };
            drawChannel (waveL, Theme::cyan.withAlpha (0.9f));
            drawChannel (waveR, Theme::magenta.withAlpha (0.6f));
        }

        // Spectrum
        {
            auto inner = bottom.reduced (6.0f);
            draw::trackedText (g, "SPECTRUM  30 HZ - 18 KHZ (LOG)   0 / -24 / -48 / -72 / -96 DB", inner.removeFromTop (14.0f), juce::Justification::centredLeft, Theme::captionFont (9.0f), Theme::textSecondary);
            for (int i = 1; i < 4; ++i)
            {
                const float y = inner.getY() + inner.getHeight() * (float) i / 4.0f;
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.drawLine (inner.getX(), y, inner.getRight(), y, 1.0f);
            }
            for (double f : { 100.0, 1000.0, 10000.0 })
            {
                const float u = (float) (std::log (f / 30.0) / std::log (18000.0 / 30.0));
                g.setColour (juce::Colours::white.withAlpha (0.07f));
                g.drawLine (inner.getX() + u * inner.getWidth(), inner.getY(), inner.getX() + u * inner.getWidth(), inner.getBottom(), 1.0f);
            }
            juce::Path p;
            p.startNewSubPath (inner.getX(), inner.getBottom());
            for (size_t i = 0; i < bands.size(); ++i)
            {
                const float x = inner.getX() + inner.getWidth() * (float) i / (float) (bands.size() - 1);
                p.lineTo (x, inner.getBottom() - bands[i] * inner.getHeight());
            }
            p.lineTo (inner.getRight(), inner.getBottom());
            p.closeSubPath();
            g.setColour (Theme::blue.withAlpha (0.25f));
            g.fillPath (p);
            g.setColour (Theme::cyan);
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }
    }

    static const char* stageName (Stage s)
    {
        switch (s)
        {
            case Stage::Source: return "Source"; case Stage::PostMatter: return "Post-Matter"; case Stage::PostEvolve: return "Post-Evolve";
            case Stage::PostFracture: return "Post-Fracture"; case Stage::PostSpace: return "Post-Space"; case Stage::Master: return "Master";
            default: return "?";
        }
    }

private:
    Diagnostics& diag;
    Stage stage = Stage::Master;
    std::array<float, 2048> waveL {}, waveR {}, mono {};
    std::array<float, 256> bands {};
    SpectrumAnalyzer analyzer;
};

//==============================================================================
/** Matter node list and frequency distribution of the focus voice. */
class DSPLabView::NodeInspector : public juce::Component
{
public:
    void set (const DiagnosticSnapshot& s) { snap = s; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto plot = b.removeFromTop (b.getHeight() * 0.42f).reduced (0.0f, 3.0f);
        auto table = b.reduced (0.0f, 3.0f);
        draw::insetSurface (g, plot, 6.0f);
        draw::insetSurface (g, table, 6.0f);

        // Frequency distribution (log axis): target = dim, actual = bright, height = energy/weight.
        {
            auto inner = plot.reduced (6.0f);
            juce::String title = "MATTER DISTRIBUTION  voice " + juce::String (snap.focusVoice) + "  note " + juce::String (snap.focusNote)
                               + "  nodes " + juce::String (snap.activeNodes) + "/" + juce::String (snap.numNodes) + "  clusters " + juce::String (snap.clusterCount)
                               + "  seed " + juce::String (snap.topologySeed);
            draw::trackedText (g, title.toUpperCase(), inner.removeFromTop (14.0f), juce::Justification::centredLeft, Theme::captionFont (9.0f), Theme::textSecondary);
            for (double f : { 100.0, 1000.0, 10000.0 })
            {
                const float u = (float) (std::log (f / 20.0) / std::log (20000.0 / 20.0));
                g.setColour (juce::Colours::white.withAlpha (0.07f));
                g.drawLine (inner.getX() + u * inner.getWidth(), inner.getY(), inner.getX() + u * inner.getWidth(), inner.getBottom(), 1.0f);
            }
            for (int i = 0; i < snap.numNodes; ++i)
            {
                const auto& n = snap.nodes[i];
                if (n.frequency <= 0.0f) continue;
                const float ut = (float) (std::log (juce::jmax (20.0f, n.targetFrequency) / 20.0f) / std::log (1000.0f));
                const float ua = (float) (std::log (juce::jmax (20.0f, n.frequency) / 20.0f) / std::log (1000.0f));
                const float hw = juce::jlimit (0.05f, 1.0f, n.weight);
                const float he = juce::jlimit (0.0f, 1.0f, n.energy * 8.0f);
                g.setColour (Theme::textDim);
                g.drawLine (inner.getX() + ut * inner.getWidth(), inner.getBottom(), inner.getX() + ut * inner.getWidth(), inner.getBottom() - hw * inner.getHeight() * 0.9f, 1.0f);
                g.setColour ((n.cluster % 2 == 0 ? Theme::cyan : Theme::magenta).withAlpha (0.4f + 0.6f * he));
                g.drawLine (inner.getX() + ua * inner.getWidth(), inner.getBottom(), inner.getX() + ua * inner.getWidth(), inner.getBottom() - juce::jmax (0.02f, he) * inner.getHeight() * 0.9f, 1.5f);
            }
        }

        // Table
        {
            auto inner = table.reduced (6.0f);
            g.setFont (Theme::font (10.5f));
            const juce::StringArray cols { "#", "FREQ", "TARGET", "RATIO", "ENERGY", "WEIGHT", "DAMP", "PAN", "NL", "CL", "EDGES", "ON" };
            const float cw = inner.getWidth() / (float) cols.size();
            auto header = inner.removeFromTop (14.0f);
            g.setColour (Theme::cyan);
            for (int c = 0; c < cols.size(); ++c)
                g.drawText (cols[c], header.withX (header.getX() + cw * (float) c).withWidth (cw), juce::Justification::centredLeft, false);
            const float rowH = 13.0f;
            for (int i = 0; i < snap.numNodes; ++i)
            {
                if (inner.getHeight() < rowH) break;
                auto row = inner.removeFromTop (rowH);
                const auto& n = snap.nodes[i];
                const juce::StringArray cells { juce::String (i), juce::String (n.frequency, 1), juce::String (n.targetFrequency, 1),
                                                juce::String (n.targetFrequency > 0.0f && snap.focusNote >= 0 ? n.targetFrequency / (float) midiNoteToHz (snap.focusNote) : 0.0f, 3),
                                                juce::String (n.energy, 4), juce::String (n.weight, 2), juce::String (n.damping, 4),
                                                juce::String (n.pan, 2), juce::String (n.nonlinearity, 2), juce::String ((int) n.cluster),
                                                juce::String ((int) n.couplingCount), n.active ? "*" : "" };
                g.setColour (n.active ? Theme::textPrimary : Theme::textDim);
                for (int c = 0; c < cells.size(); ++c)
                    g.drawText (cells[c], row.withX (row.getX() + cw * (float) c).withWidth (cw), juce::Justification::centredLeft, false);
            }
        }
    }

private:
    DiagnosticSnapshot snap;
};

//==============================================================================
DSPLabView::DSPLabView (AntiMatrProcessor& p)
    : processor (p), keyboard (p.keyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    for (auto name : { "OVERVIEW", "SOURCE", "MATTER", "EVOLVE", "FRACTURE", "MOD", "SPACE", "PERFORMANCE", "SAFETY" })
        tabs.addTab (name, Theme::panelTop, -1);
    tabs.setColour (juce::TabbedButtonBar::tabTextColourId, Theme::textSecondary);
    tabs.setColour (juce::TabbedButtonBar::frontTextColourId, Theme::textPrimary);
    addAndMakeVisible (tabs);

    engineTable  = std::make_unique<TableView> ("Engine");
    perfTable    = std::make_unique<TableView> ("CPU per subsystem (% of block budget: moving / avg / peak)");
    safetyTable  = std::make_unique<TableView> ("Safety counters");
    controlTable = std::make_unique<TableView> ("Stage levels (RMS / peak dBFS)");
    signal = std::make_unique<SignalInspector> (processor.diagnostics());
    nodes  = std::make_unique<NodeInspector>();
    for (auto* c : std::initializer_list<juce::Component*> { engineTable.get(), perfTable.get(), safetyTable.get(), controlTable.get(), signal.get(), nodes.get() })
        addAndMakeVisible (c);

    int id = 1;
    for (auto s : { "Source", "Post-Matter", "Post-Evolve", "Post-Fracture", "Post-Space", "Master" }) stageBox.addItem (s, id++);
    stageBox.setSelectedId (6, juce::dontSendNotification);
    stageBox.onChange = [this] { signal->setStage ((Stage) (stageBox.getSelectedId() - 1)); };
    addAndMakeVisible (stageBox);

    id = 1;
    for (auto s : { "FULL SYNTH", "SOURCE ONLY", "MATTER ONLY", "MATTER + EVOLVE" }) dryModeBox.addItem (s, id++);
    dryModeBox.setSelectedId (1, juce::dontSendNotification);
    dryModeBox.onChange = [this] { processor.diagnostics().dev.dryMode.store (dryModeBox.getSelectedId() - 1); };
    addAndMakeVisible (dryModeBox);

    focusBox.addItem ("Focus: latest voice", 1);
    for (int i = 0; i < kMaxVoices; ++i) focusBox.addItem ("Focus: voice " + juce::String (i), i + 2);
    focusBox.setSelectedId (1, juce::dontSendNotification);
    focusBox.onChange = [this] { processor.diagnostics().dev.focusVoice.store (focusBox.getSelectedId() - 2); };
    addAndMakeVisible (focusBox);

    auto& dev = processor.diagnostics().dev;
    bypassEvolve.onClick   = [this, &dev] { dev.bypassEvolve.store (bypassEvolve.getToggleState()); };
    bypassFracture.onClick = [this, &dev] { dev.bypassFracture.store (bypassFracture.getToggleState()); };
    bypassSpace.onClick    = [this, &dev] { dev.bypassSpace.store (bypassSpace.getToggleState()); };
    profiling.setToggleState (true, juce::dontSendNotification);
    profiling.onClick      = [this, &dev] { dev.profiling.store (profiling.getToggleState()); };
    resetSafety.onClick    = [this] { processor.diagnostics().safety.reset(); processor.diagnostics().profiler.reset(); };
    exportReport.onClick   = [this]
    {
        auto file = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("antimatr-diagnostics.json");
        file.replaceWithText (buildReport());
        eventLog.moveCaretToEnd();
        eventLog.insertTextAtCaret ("Report written: " + file.getFullPathName() + "\n");
    };
    clearLog.onClick = [this] { eventLog.clear(); logLines = 0; };
    for (auto* c : std::initializer_list<juce::Component*> { &bypassEvolve, &bypassFracture, &bypassSpace, &profiling, &resetSafety, &exportReport, &clearLog })
        addAndMakeVisible (c);

    eventLog.setMultiLine (true);
    eventLog.setReadOnly (true);
    eventLog.setScrollbarsShown (true);
    eventLog.setFont (Theme::font (11.0f));
    eventLog.setColour (juce::TextEditor::backgroundColourId, Theme::panelInset);
    addAndMakeVisible (eventLog);

    keyboard.setAvailableRange (24, 96);
    addAndMakeVisible (keyboard);

    tabs.addChangeListener (nullptr);
    selectTab (0);
}

DSPLabView::~DSPLabView() { stopTimer(); }

void DSPLabView::visibilityChanged()
{
    if (isShowing()) startTimerHz (20); else stopTimer();
}

void DSPLabView::parentHierarchyChanged()
{
    if (isShowing() && ! isTimerRunning()) startTimerHz (20);
}

void DSPLabView::selectTab (int index)
{
    index = juce::jlimit (0, juce::jmax (0, tabs.getNumTabs() - 1), index);
    if (tabs.getCurrentTabIndex() != index) tabs.setCurrentTabIndex (index, juce::dontSendNotification);
    currentTab = index;
    const bool matter = index == 2 || index == 3;
    nodes->setVisible (matter || index == 0);
    signal->setVisible (! matter || index == 0);
    resized();
}

void DSPLabView::timerCallback()
{
    if (! isShowing()) return;
    if (tabs.getCurrentTabIndex() != currentTab) selectTab (tabs.getCurrentTabIndex());

    processor.diagnostics().diagnosticSnapshots.read (snapshot);
    signal->refresh (processor.engine().sampleRate());
    nodes->set (snapshot);
    refreshTables();

    processor.diagnostics().events.drain ([this] (const EngineEvent& e)
    {
        if (logLines > 400) { eventLog.clear(); logLines = 0; }
        eventLog.moveCaretToEnd();
        eventLog.insertTextAtCaret (EngineEventQueue::describe (e) + "\n");
        ++logLines;
    });
}

void DSPLabView::refreshTables()
{
    const auto& s = snapshot;
    static const char* qualityNames[] = { "ECO", "NORMAL", "HIGH", "ULTRA" };
    static const char* dryNames[] = { "FULL", "SOURCE ONLY", "MATTER ONLY", "MATTER+EVOLVE" };
    engineTable->set ({
        { "Sample rate", juce::String (s.sampleRate, 0) + " Hz" },
        { "Block", juce::String (s.blockSize) },
        { "Quality", qualityNames[juce::jlimit (0, 3, (int) s.quality)] },
        { "Dry mode", dryNames[juce::jlimit (0, 3, (int) s.dryMode)] },
        { "Voices", juce::String (s.activeVoices) + " / " + juce::String (s.maxVoices) },
        { "Latency", juce::String (s.latencySamples) + " smp" },
        { "Sample clock", juce::String (s.sampleTime) },
        { "Focus voice / note", juce::String (s.focusVoice) + " / " + juce::String (s.focusNote) },
        { "Matter nodes", juce::String (s.activeNodes) + " / " + juce::String (s.numNodes) },
        { "Clusters", juce::String (s.clusterCount) },
        { "Topology seed", juce::String (s.topologySeed) },
        { "Matter energy", juce::String (s.matterEnergy, 4) },
        { "Material A/B/blend", juce::String ((int) s.materialA) + " / " + juce::String ((int) s.materialB) + " / " + juce::String (s.materialBlend, 2) },
        { "Fracture FFT/hop", juce::String (s.fractureFFTSize) + " / " + juce::String (s.fractureHop) },
        { "Events dropped", juce::String (s.eventsDropped) },
    });

    std::vector<std::pair<juce::String, juce::String>> perf;
    for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i)
        perf.push_back ({ subsystemName ((Subsystem) i), juce::String (s.perf.movingPercent[i], 2) + " / " + juce::String (s.perf.avgPercent[i], 2) + " / " + juce::String (s.perf.peakPercent[i], 2) });
    perf.push_back ({ "TOTAL", juce::String (s.perf.totalMovingPercent, 2) + " / " + juce::String (s.perf.totalAvgPercent, 2) + " / " + juce::String (s.perf.totalPeakPercent, 2) });
    perf.push_back ({ "Last block / budget", juce::String (s.perf.lastBlockMicros, 1) + " / " + juce::String (s.perf.budgetMicros, 1) + " us" });
    perf.push_back ({ "Overruns / blocks", juce::String (s.perf.overruns) + " / " + juce::String (s.perf.blocksMeasured) });
    perfTable->set (perf);

    std::vector<std::pair<juce::String, juce::String>> safety;
    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
    {
        juce::String v (s.safety.counts[i]);
        if (s.safety.counts[i] > 0) v << "  [" << subsystemName ((Subsystem) s.safety.lastSubsystem[i]) << (s.safety.lastVoice[i] >= 0 ? " v" + juce::String (s.safety.lastVoice[i]) : "") << "]";
        safety.push_back ({ SafetyMonitor::eventName ((SafetyEvent) i), v });
    }
    safetyTable->set (safety);

    std::vector<std::pair<juce::String, juce::String>> levels;
    for (int i = 0; i < (int) Stage::Count; ++i)
        levels.push_back ({ SignalInspector::stageName ((Stage) i), juce::String (gainToDb (s.stages[i].rms), 1) + " / " + juce::String (gainToDb (s.stages[i].peak), 1) });
    controlTable->set (levels);
}

juce::String DSPLabView::buildReport() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("build", ANTIMATR_VERSION_STRING);
    root->setProperty ("os", juce::SystemStats::getOperatingSystemName());
    root->setProperty ("sampleRate", snapshot.sampleRate);
    root->setProperty ("blockSize", snapshot.blockSize);
    root->setProperty ("quality", (int) snapshot.quality);
    root->setProperty ("voices", snapshot.activeVoices);
    root->setProperty ("maxVoices", snapshot.maxVoices);
    root->setProperty ("preset", processor.currentPresetName());
    root->setProperty ("topologySeed", (int) snapshot.topologySeed);
    root->setProperty ("materialA", (int) snapshot.materialA);
    root->setProperty ("materialB", (int) snapshot.materialB);

    auto* cpu = new juce::DynamicObject();
    for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i)
    {
        auto* sub = new juce::DynamicObject();
        sub->setProperty ("avg", snapshot.perf.avgPercent[i]);
        sub->setProperty ("peak", snapshot.perf.peakPercent[i]);
        sub->setProperty ("moving", snapshot.perf.movingPercent[i]);
        cpu->setProperty (subsystemName ((Subsystem) i), juce::var (sub));
    }
    cpu->setProperty ("totalAvg", snapshot.perf.totalAvgPercent);
    cpu->setProperty ("totalPeak", snapshot.perf.totalPeakPercent);
    root->setProperty ("cpu", juce::var (cpu));

    auto* safety = new juce::DynamicObject();
    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i) safety->setProperty (SafetyMonitor::eventName ((SafetyEvent) i), (int) snapshot.safety.counts[i]);
    root->setProperty ("safety", juce::var (safety));

    auto* params = new juce::DynamicObject();
    const auto values = processor.currentParamValues();
    for (const auto& d : ParameterRegistry::all()) params->setProperty (d.id, values[(size_t) paramIndex (d.param)]);
    root->setProperty ("parameters", juce::var (params));

    return juce::JSON::toString (juce::var (root));
}

void DSPLabView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
    draw::trackedText (g, "DSP LAB - INTERNAL ENGINEERING WORKSPACE", getLocalBounds().toFloat().removeFromTop (22.0f).withTrimmedLeft (10.0f),
                       juce::Justification::centredLeft, Theme::captionFont (9.0f), Theme::amber);
}

void DSPLabView::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (18);
    tabs.setBounds (area.removeFromTop (26));
    area.removeFromTop (6);

    auto bottom = area.removeFromBottom (juce::jmax (150, area.getHeight() / 4));
    area.removeFromBottom (6);

    auto left = area.removeFromLeft (juce::jmax (220, area.getWidth() / 5));
    area.removeFromLeft (6);
    auto right = area.removeFromRight (juce::jmax (260, area.getWidth() / 4));
    area.removeFromRight (6);

    engineTable->setBounds (left.removeFromTop (left.getHeight() * 55 / 100));
    left.removeFromTop (6);
    controlTable->setBounds (left);

    perfTable->setBounds (right.removeFromTop (right.getHeight() * 55 / 100));
    right.removeFromTop (6);
    safetyTable->setBounds (right);

    auto centre = area;
    auto stageRow = centre.removeFromTop (24);
    stageBox.setBounds (stageRow.removeFromLeft (150));
    stageRow.removeFromLeft (6);
    focusBox.setBounds (stageRow.removeFromLeft (150));
    centre.removeFromTop (4);
    if (nodes->isVisible() && signal->isVisible())
    {
        signal->setBounds (centre.removeFromTop (centre.getHeight() / 2));
        nodes->setBounds (centre);
    }
    else if (nodes->isVisible()) nodes->setBounds (centre);
    else signal->setBounds (centre);

    auto controls = bottom.removeFromLeft (juce::jmax (220, bottom.getWidth() / 4));
    bottom.removeFromLeft (6);
    dryModeBox.setBounds (controls.removeFromTop (24));
    controls.removeFromTop (4);
    auto row = controls.removeFromTop (22);
    bypassEvolve.setBounds (row.removeFromLeft (row.getWidth() / 2)); bypassFracture.setBounds (row);
    row = controls.removeFromTop (22);
    bypassSpace.setBounds (row.removeFromLeft (row.getWidth() / 2)); profiling.setBounds (row);
    controls.removeFromTop (4);
    row = controls.removeFromTop (24);
    resetSafety.setBounds (row.removeFromLeft (row.getWidth() / 3).reduced (2, 0));
    exportReport.setBounds (row.removeFromLeft (row.getWidth() / 2).reduced (2, 0));
    clearLog.setBounds (row.reduced (2, 0));

    keyboard.setBounds (bottom.removeFromBottom (60));
    bottom.removeFromBottom (6);
    eventLog.setBounds (bottom);
}

} // namespace am::dev
