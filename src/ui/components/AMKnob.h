#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    The ANTI-MATR knob: dark base, thin value arc, indicator, controlled glow,
    label. Hover shows the precise value, double-click resets, shift-drag is
    fine, right-click opens a context menu. The whole component is the hit
    target so knobs stay easy to grab at small sizes.

    Attach to a host parameter with juce::AudioProcessorValueTreeState::SliderAttachment.
*/
class AMKnob : public juce::Slider
{
public:
    explicit AMKnob (const juce::String& label = {}, juce::Colour accent = Theme::cyan);

    void setLabel (const juce::String& text) { label = text; repaint(); }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    juce::Colour getAccent() const noexcept { return accent; }

    /** Bipolar knobs draw their arc from the centre. */
    void setBipolar (bool b) { bipolar = b; repaint(); }

    /** Activity 0..1 adds glow (e.g. modulation or audio energy). */
    void setActivity (float a) { if (std::abs (a - activity) > 0.01f) { activity = a; repaint(); } }

    /** Optional callback for the right-click menu ("Reset" is always present). */
    std::function<void (juce::PopupMenu&)> onContextMenu;

    void paint (juce::Graphics& g) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    juce::Rectangle<float> knobBounds() const;

private:
    void showContextMenu();

    juce::String label;
    juce::Colour accent;
    bool bipolar = false;
    bool hovering = false;
    bool dragging = false;
    float activity = 0.0f;
};

} // namespace am::ui
