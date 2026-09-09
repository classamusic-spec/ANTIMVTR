#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <vector>

namespace am::ui
{

/** A value that eases toward its target; driven by Animator. */
struct Eased
{
    float value = 0.0f;
    float target = 0.0f;

    /** Moves toward the target. Returns true while still moving. */
    bool settle (float rate) noexcept
    {
        const float d = target - value;
        if (std::abs (d) < 0.004f)
        {
            if (value == target) return false;
            value = target;
            return true;
        }
        value += d * rate;
        return true;
    }

    void snap (float v) noexcept { value = target = v; }
};

/**
    Cheap timer-eased animation for hover / selection transitions. The timer
    only runs while at least one value is still moving, so idle components
    cost nothing. 30 Hz is plenty for interface transitions.
*/
class Animator : private juce::Timer
{
public:
    Animator (juce::Component& ownerComponent, std::initializer_list<Eased*> values, float easeRate = 0.28f, int hz = 30)
        : owner (ownerComponent), items (values), rate (easeRate), frequency (hz) {}

    ~Animator() override { stopTimer(); }

    /** Call after changing any target. */
    void kick()
    {
        if (! isTimerRunning()) startTimerHz (frequency);
    }

    /** Sets a target and starts the timer if needed. */
    void animate (Eased& e, float target)
    {
        if (e.target == target) return;
        e.target = target;
        kick();
    }

private:
    void timerCallback() override
    {
        bool moving = false;
        for (auto* e : items) moving = e->settle (rate) || moving;
        owner.repaint();
        if (! moving) stopTimer();
    }

    juce::Component& owner;
    std::vector<Eased*> items;
    float rate;
    int frequency;
};

} // namespace am::ui
