#include "AMModAssign.h"

namespace am::ui
{

ModAssign& ModAssign::get()
{
    static ModAssign instance;
    return instance;
}

void ModAssign::arm (ModSource s)
{
    const auto next = (s == armedSource || s == ModSource::None) ? ModSource::None : s;
    if (next == armedSource) return;
    armedSource = next;
    sendChangeMessage();
}

void ModAssign::cancel()
{
    if (armedSource == ModSource::None) return;
    armedSource = ModSource::None;
    sendChangeMessage();
}

bool ModAssign::assignTo (Param target)
{
    if (! isArmed()) return false;
    const auto s = armedSource;
    armedSource = ModSource::None;
    const bool accepted = onAssign != nullptr && onAssign (s, target);
    sendChangeMessage();
    return accepted;
}

} // namespace am::ui
