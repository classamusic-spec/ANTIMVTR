#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMModRing.h"
#include "AMModAssign.h"
#include "dsp/mod/ModulationSnapshot.h"

#include <optional>

namespace am::ui
{

/**
    The ANTI-MATR knob: sphere-like dark base with rim light, a thin value
    arc in the section accent, a fine white indicator and a soft controlled
    glow. Hover shows the precise value, double-click resets, shift-drag is
    fine, right-click opens a context menu. The whole component is the hit
    target so knobs stay easy to grab at small sizes.

    Attach to a host parameter with juce::AudioProcessorValueTreeState::SliderAttachment.
    modRing() exposes the modulation display (base / range / current).
*/
class AMKnob : public juce::Slider,
               private juce::ChangeListener
{
public:
    explicit AMKnob (const juce::String& label = {}, juce::Colour accent = Theme::cyan);
    ~AMKnob() override;

    void setLabel (const juce::String& text);
    void setAccent (juce::Colour c);
    juce::Colour getAccent() const noexcept { return accent; }

    /** Bipolar knobs draw their arc from the centre. */
    void setBipolar (bool b) { bipolar = b; repaint(); }

    /** Activity 0..1 adds glow (e.g. modulation or audio energy). */
    void setActivity (float a) { if (std::abs (a - activity) > 0.02f) { activity = a; repaint(); } }

    /** Larger knobs (hero controls) get a slightly bolder arc and label. */
    void setHero (bool h) { hero = h; repaint(); }

    /** Modulation ring (base value follows the knob automatically). */
    AMModRing& modRing() noexcept { return ring; }

    /** The parameter this knob drives. Set it to take part in modulation assignment and display. */
    void setModTarget (std::optional<Param> p);
    std::optional<Param> getModTarget() const noexcept { return modTarget; }

    /** Draws the live modulation from the engine's snapshot. Returns false when nothing is routed here. */
    bool refreshModRing (const ModulationSnapshot& snapshot);

    /** Optional callback for the right-click menu ("Reset" is always present). */
    std::function<void (juce::PopupMenu&)> onContextMenu;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void valueChanged() override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    juce::Rectangle<float> knobBounds() const;
    float labelHeight() const;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    void showContextMenu();
    float proportion() const;
    bool isAssignTarget() const noexcept;

    juce::String label, labelUpper;
    juce::Colour accent;
    std::optional<Param> modTarget;
    bool bipolar = false;
    bool hero = false;
    bool dragging = false;
    float activity = 0.0f;
    Eased hover;
    Animator anim { *this, { &hover } };
    AMModRing ring;
};

} // namespace am::ui
