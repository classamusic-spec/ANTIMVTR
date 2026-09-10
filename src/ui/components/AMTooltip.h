#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    Floating value bubble shown next to a control while it is being edited
    (XY pad, step editor). Its look comes from the LookAndFeel's drawTooltip
    so it matches the hover tooltips of the rest of the interface.

    showFor() adds the bubble to the control's top-level parent, positions
    it next to a point in the control's coordinates and hides it after
    `holdMs` milliseconds of inactivity.
*/
class AMTooltip : public juce::Component,
                  private juce::Timer
{
public:
    AMTooltip();
    ~AMTooltip() override;

    void showFor (juce::Component& control, juce::Point<int> localPoint, const juce::String& text, int holdMs = 900);
    void hide();

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { hide(); }
    juce::String text;
    juce::Font font { juce::FontOptions() };
};


/**
    The hover tooltip window for the whole editor.

    Identical to `juce::TooltipWindow` except that a tip is only offered once
    the pointer has actually *moved onto* the control: a tip never pops up
    under a pointer that happens to be resting where a page, panel or dialog
    has just appeared. Without this a stationary pointer leaves a bubble
    floating over the interface (and over every snapshot) forever.
*/
class AMTooltipWindow : public juce::TooltipWindow
{
public:
    AMTooltipWindow (juce::Component* parent, int hoverDelayMs)
        : juce::TooltipWindow (parent, hoverDelayMs)
    {
    }

    juce::String getTipFor (juce::Component& c) override
    {
        const auto position = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
        if (! seenPointer)
        {
            seenPointer = true;
            lastPosition = position;
            lastTarget = &c;
            return {};
        }

        const bool movedNow = position.getDistanceFrom (lastPosition) > 0.5f;
        if (movedNow)
        {
            lastPosition = position;
            pointedAtTarget = true;
        }
        if (&c != lastTarget)
        {
            lastTarget = &c;
            if (! movedNow) pointedAtTarget = false;    // the control moved under the pointer, not the other way round
        }
        return pointedAtTarget ? juce::TooltipWindow::getTipFor (c) : juce::String();
    }

private:
    juce::Point<float> lastPosition;
    juce::Component* lastTarget = nullptr;
    bool seenPointer = false, pointedAtTarget = false;
};

} // namespace am::ui
