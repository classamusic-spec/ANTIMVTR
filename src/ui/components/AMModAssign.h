#pragma once

#include "state/ModRouting.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace am::ui
{

/**
    ASSIGN MODE — "pick a source, then click a knob".

    The ROUTINGS panel arms a source; every knob bound to a modulatable
    parameter *in that editor* then lights up and takes the next click as a
    destination instead of starting a drag.

    One instance serves the whole process, so it remembers which editor armed
    it: a second plug-in instance never lights up or steals the assignment.
    The panel owns the actual routing change through the callback it passes to
    `arm()`; this class only carries the intent.
*/
class ModAssign : public juce::ChangeBroadcaster
{
public:
    static ModAssign& get();

    using AssignFn = std::function<bool (ModSource, Param)>;

    /** Arms assignment from `s` for the editor `owner` belongs to. Re-arming the same source cancels. */
    void arm (ModSource s, juce::Component* owner, AssignFn assign);
    void cancel();
    /** Cancels only if `owner`'s editor is the one that armed (called when the panel goes away). */
    void cancelFor (juce::Component* owner);

    bool isArmed() const noexcept { return armedSource != ModSource::None; }
    ModSource source() const noexcept { return armedSource; }

    /** True when `c` lives in the editor that armed assignment. */
    bool isArmedFor (const juce::Component* c) const noexcept;

    /** Called by a bound control when it is clicked. Returns true if it consumed the click. */
    bool assignTo (Param target, const juce::Component* clicked);

private:
    ModAssign() = default;
    static const juce::Component* rootOf (const juce::Component* c) noexcept;

    ModSource armedSource = ModSource::None;
    juce::Component::SafePointer<juce::Component> armedRoot;
    AssignFn assignFn;
};

} // namespace am::ui
