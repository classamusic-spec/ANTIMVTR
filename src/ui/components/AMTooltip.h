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

} // namespace am::ui
