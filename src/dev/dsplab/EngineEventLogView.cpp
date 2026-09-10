#include "EngineEventLogView.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    juce::Colour eventColour (EngineEventType t) noexcept
    {
        switch (t)
        {
            case EngineEventType::SafetyEvent:  return Theme::magenta;
            case EngineEventType::SafetyReset:  return Theme::amber;
            case EngineEventType::VoiceStolen:  return Theme::amber;
            case EngineEventType::VoiceStarted: return Theme::cyan;
            case EngineEventType::VoiceEnded:   return Theme::textSecondary;
            case EngineEventType::PresetLoaded: return Theme::violet;
            case EngineEventType::EnginePrepared:
            case EngineEventType::EngineReset:  return Theme::blue;
            default:                            return Theme::textPrimary;
        }
    }
}

EngineEventLogView::EngineEventLogView()
{
    typeFilter.addItem ("ALL TYPES", 1);
    for (int i = 0; i < (int) EngineEventType::Count; ++i)
        typeFilter.addItem (juce::String (EngineEventQueue::typeName ((EngineEventType) i)).toUpperCase(), i + 2);
    typeFilter.setSelectedId (1, juce::dontSendNotification);
    typeFilter.onChange = [this] { dirty = true; };
    styleCombo (typeFilter);
    addAndMakeVisible (typeFilter);

    subsystemFilter.addItem ("ALL SUBSYSTEMS", 1);
    for (int i = 0; i < (int) Subsystem::Count; ++i)
        subsystemFilter.addItem (juce::String (subsystemName ((Subsystem) i)).toUpperCase(), i + 2);
    subsystemFilter.setSelectedId (1, juce::dontSendNotification);
    subsystemFilter.onChange = [this] { dirty = true; };
    styleCombo (subsystemFilter);
    addAndMakeVisible (subsystemFilter);

    textFilter.setTextToShowWhenEmpty ("filter text...", Theme::textDim);
    textFilter.setFont (Theme::font (11.5f));
    textFilter.setColour (juce::TextEditor::backgroundColourId, Theme::panelInset);
    textFilter.setColour (juce::TextEditor::outlineColourId, Theme::border);
    textFilter.setColour (juce::TextEditor::textColourId, Theme::textPrimary);
    textFilter.onTextChange = [this] { dirty = true; };
    addAndMakeVisible (textFilter);

    styleToggle (pause, Theme::amber);
    addAndMakeVisible (pause);

    styleButton (clear);
    clear.onClick = [this]
    {
        entries.clear();
        typeCounts.fill (0);
        totalSeen = 0;
        dirty = true;
    };
    addAndMakeVisible (clear);

    table.setColumns ({
        { "TIME s",  70, true },  { "TYPE", 150, false }, { "SUBSYSTEM", 100, false },
        { "VOICE",   52, true },  { "A", 70, true },      { "F", 80, true },
        { "DESCRIPTION", 320, false }
    });
    tablePanel.addAndMakeVisible (table);
    addAndMakeVisible (tablePanel);

    stats.setRowHeight (14.0f);
    stats.setLabelWidthFraction (0.62f);
    addAndMakeVisible (stats);
}

void EngineEventLogView::addEvent (const EngineEvent& e, uint64_t sampleRate)
{
    Entry entry;
    entry.event = e;
    entry.seconds = sampleRate > 0 ? (double) e.sampleTime / (double) sampleRate : 0.0;
    entry.text = EngineEventQueue::describe (e);
    entries.push_back (std::move (entry));

    while (entries.size() > kMaxEntries)
        entries.pop_front();

    if ((int) e.type < (int) EngineEventType::Count)
        ++typeCounts[(size_t) e.type];
    ++totalSeen;
    dirty = true;
}

bool EngineEventLogView::matches (const Entry& e) const
{
    if (typeFilter.getSelectedId() > 1 && (int) e.event.type != typeFilter.getSelectedId() - 2)
        return false;
    if (subsystemFilter.getSelectedId() > 1 && (int) e.event.subsystem != subsystemFilter.getSelectedId() - 2)
        return false;

    const auto text = textFilter.getText().trim();
    if (text.isNotEmpty() && ! e.text.containsIgnoreCase (text))
        return false;

    return true;
}

void EngineEventLogView::rebuild()
{
    std::vector<LabTable::Row> rows;
    rows.reserve (entries.size());

    for (const auto& e : entries)
    {
        if (! matches (e))
            continue;

        const auto colour = eventColour (e.event.type);
        rows.push_back ({
            { juce::String (e.seconds, 3), e.seconds, Theme::textDim },
            { juce::String (EngineEventQueue::typeName (e.event.type)), (double) (int) e.event.type, colour },
            { juce::String (subsystemName ((Subsystem) e.event.subsystem)), (double) e.event.subsystem, Theme::textSecondary },
            { e.event.voice >= 0 ? juce::String ((int) e.event.voice) : juce::String ("-"), (double) e.event.voice, Theme::textSecondary },
            { juce::String ((int) e.event.a), (double) e.event.a, Theme::textSecondary },
            { juce::String (e.event.f, 3), (double) e.event.f, Theme::textSecondary },
            { e.text, 0.0, Theme::textPrimary }
        });
    }

    tablePanel.setSubtitle (juce::String ((int) rows.size()) + " / " + juce::String ((int) entries.size()) + " SHOWN");
    table.setRows (std::move (rows));
    dirty = false;
}

void EngineEventLogView::updateFrame (const LabFrame& f)
{
    if (dirty && ! pause.getToggleState())
        rebuild();

    std::vector<KeyValueTable::Row> rows;
    rows.push_back ({ "Events seen", juce::String (totalSeen) });
    rows.push_back ({ "Kept in ring", juce::String ((int) entries.size()) + " / " + juce::String ((int) kMaxEntries) });
    rows.push_back ({ "Dropped by engine", juce::String (f.snapshot.eventsDropped) });
    rows.push_back ({ "Queue pending", juce::String ((int) f.diagnostics.events.pending()) });
    rows.push_back ({ "BY TYPE", "" });
    for (int i = 0; i < (int) EngineEventType::Count; ++i)
        if (typeCounts[(size_t) i] > 0)
            rows.push_back ({ juce::String (EngineEventQueue::typeName ((EngineEventType) i)),
                              juce::String (typeCounts[(size_t) i]) });

    stats.setRows (std::move (rows));
    stats.clearRowColours();
    if (f.snapshot.eventsDropped > 0)
        stats.setRowColour (2, Theme::amber);
}

void EngineEventLogView::resized()
{
    auto area = getLocalBounds();

    auto controls = area.removeFromTop (24);
    typeFilter.setBounds (controls.removeFromLeft (170));
    controls.removeFromLeft (5);
    subsystemFilter.setBounds (controls.removeFromLeft (160));
    controls.removeFromLeft (5);
    textFilter.setBounds (controls.removeFromLeft (juce::jmax (140, controls.getWidth() / 3)));
    controls.removeFromLeft (10);
    pause.setBounds (controls.removeFromLeft (80));
    clear.setBounds (controls.removeFromLeft (70).reduced (0, 1));
    area.removeFromTop (5);

    auto right = area.removeFromRight (juce::jmax (200, area.getWidth() * 24 / 100));
    area.removeFromRight (5);
    stats.setBounds (right);

    tablePanel.setBounds (area);
    table.setBounds (tablePanel.contentBounds());
}

} // namespace am::dev
