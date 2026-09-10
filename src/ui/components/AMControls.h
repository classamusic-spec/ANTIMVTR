#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"

namespace am::ui
{

/** Compact labelled on/off pill (ON glows in the accent). */
class AMToggle : public juce::Component,
                 public juce::SettableTooltipClient
{
public:
    explicit AMToggle (const juce::String& label, juce::Colour accent = Theme::cyan);

    void setToggleState (bool on, juce::NotificationType notify = juce::sendNotification);
    bool getToggleState() const noexcept { return state; }
    void setLabel (const juce::String& l) { label = l.toUpperCase(); repaint(); }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    std::function<void (bool)> onChange;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { anim.animate (hover, 1.0f); }
    void mouseExit (const juce::MouseEvent&) override { anim.animate (hover, 0.0f); }

private:
    juce::String label;
    juce::Colour accent;
    bool state = false;
    Eased hover, lit;
    Animator anim { *this, { &hover, &lit } };
};

/**
    Choice stepper: ‹ VALUE › pill with the parameter name above. Clicking
    the chevrons steps, clicking the value opens a popup with every choice.
*/
class AMChoice : public juce::Component,
                 public juce::SettableTooltipClient
{
public:
    AMChoice (const juce::String& label, juce::StringArray choices, juce::Colour accent = Theme::cyan);

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int getSelected() const noexcept { return selected; }
    void setLabel (const juce::String& l) { label = l.toUpperCase(); repaint(); }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    /** Without a label the pill fills the whole component. */
    void setShowLabel (bool b) { showLabel = b; repaint(); }
    std::function<void (int)> onChange;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { anim.animate (hover, 1.0f); }
    void mouseExit (const juce::MouseEvent&) override { anim.animate (hover, 0.0f); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

private:
    juce::Rectangle<float> pillBounds() const;
    juce::String label;
    juce::StringArray choices;
    juce::Colour accent;
    int selected = 0;
    bool showLabel = true;
    Eased hover;
    Animator anim { *this, { &hover } };
};

} // namespace am::ui
