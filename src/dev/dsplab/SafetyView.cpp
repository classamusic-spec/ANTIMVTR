#include "SafetyView.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    /** Categories that mean the audio itself was damaged. */
    bool isCritical (SafetyEvent e) noexcept
    {
        return e == SafetyEvent::NaN || e == SafetyEvent::Infinity || e == SafetyEvent::HardClip
            || e == SafetyEvent::InvalidCoefficient || e == SafetyEvent::InvalidFrequency;
    }
}

SafetyView::SafetyView()
{
    counters.setColumns ({
        { "EVENT", 150, false }, { "COUNT", 70, true },
        { "LAST SUBSYSTEM", 120, false }, { "LAST VOICE", 76, true }
    });
    counters.setDefaultSort (2, false);    // loudest problem first
    counters.setRowColourFn ([] (int row) -> juce::Colour
    {
        return isCritical ((SafetyEvent) row) ? Theme::magenta.withAlpha (0.05f) : juce::Colours::transparentBlack;
    });
    tablePanel.addAndMakeVisible (counters);
    addAndMakeVisible (tablePanel);

    summary.setRowHeight (14.0f);
    summary.setLabelWidthFraction (0.58f);
    addAndMakeVisible (summary);

    log.setMultiLine (true);
    log.setReadOnly (true);
    log.setScrollbarsShown (true);
    log.setFont (Theme::font (10.5f));
    log.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    log.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    log.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    log.setColour (juce::TextEditor::textColourId, Theme::textSecondary);
    logPanel.addAndMakeVisible (log);
    addAndMakeVisible (logPanel);

    styleButton (resetCounters, Theme::amber);
    styleButton (clearLog);
    resetCounters.onClick = [this]
    {
        if (diagnostics != nullptr) diagnostics->safety.reset();
    };
    clearLog.onClick = [this] { log.clear(); logLines = 0; };
    controls.addAndMakeVisible (resetCounters);
    controls.addAndMakeVisible (clearLog);
    addAndMakeVisible (controls);
}

void SafetyView::addEvent (const EngineEvent& e)
{
    if (logLines > 500)
    {
        log.clear();
        logLines = 0;
        log.insertTextAtCaret ("[log trimmed]\n");
    }
    log.moveCaretToEnd();
    log.insertTextAtCaret (EngineEventQueue::describe (e) + "\n");
    ++logLines;
}

void SafetyView::rebuildCounters (const DiagnosticSnapshot& s)
{
    std::vector<LabTable::Row> rows;
    rows.reserve ((size_t) SafetyMonitor::kNumEvents);

    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
    {
        const auto event = (SafetyEvent) i;
        const uint32_t count = s.safety.counts[i];
        const auto colour = count == 0 ? Theme::textDim
                                       : (isCritical (event) ? Theme::magenta : Theme::amber);

        rows.push_back ({
            { juce::String (SafetyMonitor::eventName (event)), 0.0, count == 0 ? Theme::textSecondary : colour },
            { juce::String ((int) count), (double) count, colour },
            { count > 0 ? juce::String (subsystemName ((Subsystem) s.safety.lastSubsystem[i])) : juce::String ("-"),
              0.0, count > 0 ? Theme::textSecondary : Theme::textDim },
            { count > 0 && s.safety.lastVoice[i] >= 0 ? juce::String ((int) s.safety.lastVoice[i]) : juce::String ("-"),
              (double) s.safety.lastVoice[i], count > 0 ? Theme::textSecondary : Theme::textDim }
        });
    }

    counters.setRows (std::move (rows));
}

void SafetyView::updateFrame (const LabFrame& f)
{
    diagnostics = &f.diagnostics;
    const auto& s = f.snapshot;

    rebuildCounters (s);

    int criticalCount = 0, warnCount = 0;
    for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
        if (s.safety.counts[i] > 0)
            (isCritical ((SafetyEvent) i) ? criticalCount : warnCount) += (int) s.safety.counts[i];

    const bool clean = s.safety.total == 0;
    summary.setRows ({
        { "Total events",    juce::String ((int) s.safety.total) },
        { "Critical",        juce::String (criticalCount) },
        { "Warnings",        juce::String (warnCount) },
        { "Since last frame",juce::String ((int) (s.safety.total > lastTotal ? s.safety.total - lastTotal : 0)) },
        { "", "" },
        { "Verdict",         clean ? "CLEAN" : (criticalCount > 0 ? "AUDIO DAMAGED" : "WARNINGS") },
        { "", "" },
        { "Events dropped",  juce::String (s.eventsDropped) },
        { "Events pending",  juce::String ((int) f.diagnostics.events.pending()) },
        { "Master peak",     juce::String (s.stages[(int) Stage::Master].peak, 4) },
        { "Master rms",      dbString (s.stages[(int) Stage::Master].rms) + " dB" },
    });

    summary.clearRowColours();
    summary.setRowColour (0, clean ? Theme::textPrimary : Theme::amber);
    summary.setRowColour (1, criticalCount > 0 ? Theme::magenta : Theme::textDim);
    summary.setRowColour (2, warnCount > 0 ? Theme::amber : Theme::textDim);
    summary.setRowColour (5, clean ? Theme::cyan : (criticalCount > 0 ? Theme::magenta : Theme::amber));
    if (s.stages[(int) Stage::Master].peak >= 0.999f)
        summary.setRowColour (9, Theme::magenta);

    lastTotal = s.safety.total;
    tablePanel.setSubtitle (clean ? "NO EVENTS" : juce::String ((int) s.safety.total) + " EVENTS");
}

void SafetyView::resized()
{
    auto area = getLocalBounds();

    auto right = area.removeFromRight (juce::jmax (215, area.getWidth() * 28 / 100));
    area.removeFromRight (5);

    controls.setBounds (right.removeFromTop (50));
    {
        auto inner = controls.contentBounds();
        resetCounters.setBounds (inner.removeFromLeft (inner.getWidth() / 2).reduced (2, 1));
        clearLog.setBounds (inner.reduced (2, 1));
    }
    right.removeFromTop (5);
    summary.setBounds (right);

    tablePanel.setBounds (area.removeFromTop (juce::jmax (170, area.getHeight() * 48 / 100)));
    counters.setBounds (tablePanel.contentBounds());
    area.removeFromTop (5);
    logPanel.setBounds (area);
    log.setBounds (logPanel.contentBounds());
}

} // namespace am::dev
