#pragma once

#include "AMDrawing.h"
#include "AMKnobArt.h"
#include "AMAnimator.h"
#include "AMModRing.h"
#include "AMModAssign.h"
#include "dsp/mod/ModulationSnapshot.h"
#include "ui/UILayout.h"

#include <optional>

namespace am::ui
{

/**
    The ANTI-MATR knob — one piece of hardware in three dresses (SPEC section 2).

      Style::CappedLit   a matte near-black moulded body with a turned metal cap,
                         ringed by amber LED dots lit up to the value. The hero.
      Style::CappedDark  the same body and cap with the ring dark, for dense
                         clusters where every ring glowing would be noise. The
                         dots warm as soon as the control is touched or active.
      Style::Plain       a quiet matte dome with a concentric groove and a notch,
                         for small and secondary controls.

    Style::Auto (the default) picks from the knob's role: the subject of a panel
    is lit, the cluster around it is dark, and a control nothing can modulate — a
    seed, a transpose, a unison count — is plain, because it is a setting rather
    than something played. Size never changes the body: a knob too small to carry
    a readable ring of dots sheds the ring and spends the room on the body, so the
    cap and the indicator survive down to the smallest control in the instrument.

    What moves, and why it moves:
      - the cap's sunburst sheen eases behind the value, so turning the knob
        makes the metal catch the light differently;
      - hover and drag lift the cap's specular, warm the dots and swap the label
        for the value;
      - the dot that has just lit flares and settles;
      - pressing sinks the body onto a tighter shadow.
    Nothing here is decoration: every channel carries value, activity,
    modulation or focus.

    Hover shows the precise value, double-click resets, shift-drag is fine,
    right-click opens a context menu. The whole component is the hit target so
    knobs stay easy to grab at small sizes.

    Attach to a host parameter with juce::AudioProcessorValueTreeState::SliderAttachment.
    modRing() exposes the modulation display (base / range / current).
*/
class AMKnob : public juce::Slider,
               private juce::ChangeListener
{
public:
    using Style = knobart::Style;

    explicit AMKnob (const juce::String& label = {}, juce::Colour accent = Theme::cyan);
    ~AMKnob() override;

    void setLabel (const juce::String& text);
    void setAccent (juce::Colour c);
    juce::Colour getAccent() const noexcept { return accent; }

    /** Which body this knob wears. Style::Auto derives it from the role and the size. */
    void setStyle (Style s);
    Style getStyle() const noexcept { return style; }
    /** The style actually drawn, with Auto resolved against the knob's current size. */
    Style effectiveStyle() const noexcept;

    /** Bipolar knobs light their ring out from the centre. */
    void setBipolar (bool b) { bipolar = b; repaint(); }

    /** Activity 0..1 warms the ring (e.g. modulation or audio energy). */
    void setActivity (float a) { if (std::abs (a - activity) > 0.02f) { activity = a; repaint(); } }
    float getActivity() const noexcept { return activity; }

    /** Larger knobs (hero controls) get the lit ring and a bolder label. */
    void setHero (bool h);
    bool isHero() const noexcept { return hero; }

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
    /** The concentric geometry actually drawn: modulation orbit, LED ring, body, cap. */
    knobart::Geometry geometry() const;
    float labelHeight() const;

    /** Everything the modulation orbit needs, read back from the ring. */
    knobart::ModView modView() const;

private:
    /**
        The modulation orbit, drawn over the knob it belongs to.

        It subclasses AMModRing purely to keep that component's data API and its
        repaint-when-the-data-moves behaviour, and replaces its paint with the
        pearl-palette art. The range is the one value AMModRing keeps to itself,
        so the knob caches it on the way through refreshModRing().
    */
    class ModOrbit : public AMModRing
    {
    public:
        explicit ModOrbit (AMKnob& k) : knob (k) {}
        void paint (juce::Graphics& g) override;
    private:
        AMKnob& knob;
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    void showContextMenu();
    float proportion() const;
    bool isAssignTarget() const noexcept;
    /** The LED at the value, and the run of lit dots around it. */
    void litRun (const knobart::Geometry& geo, float p, int& head, int& lo, int& hi) const;

    juce::String label, labelUpper;
    juce::Colour accent;
    std::optional<Param> modTarget;
    Style style = Style::Auto;
    bool bipolar = false;
    bool hero = false;
    bool dragging = false;
    float activity = 0.0f;
    float modLo = 0.0f, modHi = 0.0f;
    bool modHasRange = false;
    int flareDot = -1;

    Eased hover;   ///< pointer over the control
    Eased press;   ///< held down
    Eased sheen;   ///< the cap's highlight, easing behind the value
    Eased flare;   ///< the dot that has just lit, decaying
    Animator anim { *this, { &hover, &press, &sheen, &flare }, 0.26f };
    ModOrbit ring { *this };
};

} // namespace am::ui
