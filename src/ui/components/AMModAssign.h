#pragma once

#include "state/ModRouting.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace am::ui
{

/**
    ASSIGN MODE — "pick a source, then click a knob".

    The ROUTINGS panel arms a source; every knob bound to a modulatable
    parameter then lights up and takes the next click as a destination instead
    of starting a drag. One shared instance keeps the whole editor in step and
    lets the knobs repaint themselves when the mode changes.

    The panel owns the actual routing change through `onAssign`; this class
    only carries the intent.
*/
class ModAssign : public juce::ChangeBroadcaster
{
public:
    static ModAssign& get();

    /** Arms assignment from `s`; passing the armed source again cancels (toggle). */
    void arm (ModSource s);
    void cancel();

    bool isArmed() const noexcept { return armedSource != ModSource::None; }
    ModSource source() const noexcept { return armedSource; }

    /** Called by a bound control when it is clicked. Returns true if it consumed the click. */
    bool assignTo (Param target);

    /** Installed by the ROUTINGS panel. Returns true when the routing was accepted. */
    std::function<bool (ModSource, Param)> onAssign;

private:
    ModAssign() = default;
    ModSource armedSource = ModSource::None;
};

} // namespace am::ui
