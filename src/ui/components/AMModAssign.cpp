#include "AMModAssign.h"

namespace am::ui
{

ModAssign& ModAssign::get()
{
    static ModAssign instance;
    return instance;
}

const juce::Component* ModAssign::rootOf (const juce::Component* c) noexcept
{
    return c != nullptr ? c->getTopLevelComponent() : nullptr;
}

void ModAssign::arm (ModSource s, juce::Component* owner, AssignFn assign)
{
    auto* root = owner != nullptr ? owner->getTopLevelComponent() : nullptr;
    const bool sameSource = s == armedSource && armedRoot.getComponent() == root;
    const auto next = (s == ModSource::None || sameSource) ? ModSource::None : s;

    armedSource = next;
    armedRoot = next == ModSource::None ? nullptr : root;
    assignFn = next == ModSource::None ? nullptr : std::move (assign);
    sendChangeMessage();
}

void ModAssign::cancel()
{
    if (armedSource == ModSource::None) return;
    armedSource = ModSource::None;
    armedRoot = nullptr;
    assignFn = nullptr;
    sendChangeMessage();
}

void ModAssign::cancelFor (juce::Component* owner)
{
    if (armedRoot.getComponent() == rootOf (owner)) cancel();
}

bool ModAssign::isArmedFor (const juce::Component* c) const noexcept
{
    return isArmed() && armedRoot.getComponent() != nullptr && armedRoot.getComponent() == rootOf (c);
}

bool ModAssign::assignTo (Param target, const juce::Component* clicked)
{
    if (! isArmedFor (clicked)) return false;

    const auto s = armedSource;
    auto fn = std::move (assignFn);
    armedSource = ModSource::None;
    armedRoot = nullptr;
    assignFn = nullptr;

    const bool accepted = fn != nullptr && fn (s, target);
    sendChangeMessage();
    return accepted;
}

} // namespace am::ui
