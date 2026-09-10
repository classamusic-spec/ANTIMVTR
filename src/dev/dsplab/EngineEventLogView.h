#pragma once

#include "LabWidgets.h"

#include <deque>

namespace am::dev
{

/**
    ENGINE EVENT LOG (§87)

    The lock-free event stream from the audio thread, kept in a bounded ring
    and rendered as a filterable table: by event type, by subsystem and by
    free text. Pausing stops the view from scrolling but never stops the
    shell draining the queue, so nothing is lost while reading.
*/
class EngineEventLogView : public LabView
{
public:
    EngineEventLogView();

    /** Called by the shell for every drained event. */
    void addEvent (const EngineEvent& e, uint64_t sampleRate);

    void updateFrame (const LabFrame& f) override;
    void resized() override;

private:
    struct Entry
    {
        EngineEvent event;
        double seconds = 0.0;
        juce::String text;
    };

    void rebuild();
    bool matches (const Entry& e) const;

    static constexpr size_t kMaxEntries = 2000;

    std::deque<Entry> entries;
    LabPanel tablePanel { "Engine events" };
    LabTable table;
    juce::ComboBox typeFilter, subsystemFilter;
    juce::TextEditor textFilter;
    juce::ToggleButton pause { "Pause" };
    juce::TextButton clear { "CLEAR" };
    KeyValueTable stats { "Event statistics" };

    std::array<int, (int) EngineEventType::Count> typeCounts {};
    int totalSeen = 0;
    bool dirty = true;
};

} // namespace am::dev
