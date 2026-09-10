#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMModRing.h"
#include "AMModAssign.h"
#include "dsp/mod/ModulationSnapshot.h"
#include "ui/UILayout.h"

#include <optional>

namespace am::ui
{

/**
    Concentric geometry of a knob, derived from its square footprint.

    From the rim inwards: the modulation orbit (where AMModRing lives), a
    clear moat, the value arc, and the sphere body. Keeping the orbit
    reserved whether or not the knob is modulated means the knob never
    changes size when a routing is added.
*/
struct KnobRings
{
    float ringStroke = 1.1f;     ///< stroke of the modulation orbit
    float trackWidth = 1.4f;     ///< stroke of the value arc
    juce::Rectangle<float> orbit, arc, body;

    /** Builds the geometry for a footprint (the largest centred square is used). */
    static KnobRings forFootprint (juce::Rectangle<float> footprint, bool hero) noexcept
    {
        KnobRings r;
        const float d = juce::jmin (footprint.getWidth(), footprint.getHeight());
        const auto square = footprint.withSizeKeepingCentre (d, d);
        const auto radii = layout::knobRadii (d, hero);
        r.ringStroke = radii.ringStroke;
        r.trackWidth = radii.trackWidth;
        r.orbit = square.withSizeKeepingCentre (radii.orbit, radii.orbit);
        r.arc   = square.withSizeKeepingCentre (radii.arc, radii.arc);
        r.body  = square.withSizeKeepingCentre (radii.body, radii.body);
        return r;
    }

    /** Gap in pixels between the outside of the value arc and the middle of the orbit. */
    float moat() const noexcept { return (orbit.getWidth() - arc.getWidth()) * 0.5f - trackWidth * 0.5f; }
};

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
    void visibilityChanged() override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    juce::Rectangle<float> knobBounds() const;
    KnobRings rings() const { return KnobRings::forFootprint (knobBounds(), hero); }
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
