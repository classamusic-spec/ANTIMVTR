#include "AMKnob.h"

namespace am::ui
{

AMKnob::AMKnob (const juce::String& labelText, juce::Colour accentColour)
    : label (labelText), labelUpper (labelText.toUpperCase()), accent (accentColour)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setMouseDragSensitivity (240);
    setVelocityBasedMode (false);
    setDoubleClickReturnValue (true, 0.0);
    setScrollWheelEnabled (true);
    setWantsKeyboardFocus (false);
    setRepaintsOnMouseActivity (false);
    ring.setAngles (getRotaryParameters().startAngleRadians, getRotaryParameters().endAngleRadians);
    ring.setAccent (Theme::amber);
    addAndMakeVisible (ring);
}

AMKnob::~AMKnob()
{
    if (modTarget.has_value()) ModAssign::get().removeChangeListener (this);
}

void AMKnob::setModTarget (std::optional<Param> p)
{
    if (p.has_value() == modTarget.has_value() && (! p.has_value() || *p == *modTarget)) return;
    if (modTarget.has_value() && ! p.has_value()) ModAssign::get().removeChangeListener (this);
    if (! modTarget.has_value() && p.has_value()) ModAssign::get().addChangeListener (this);
    modTarget = p;
    repaint();
}

bool AMKnob::isAssignTarget() const noexcept
{
    return modTarget.has_value() && ModAssign::get().isArmedFor (this);
}

bool AMKnob::refreshModRing (const ModulationSnapshot& snapshot)
{
    if (! modTarget.has_value()) return false;
    const int index = paramIndex (*modTarget);
    if (index < 0 || index >= kNumParams || snapshot.targeted[index] == 0) { ring.clear(); return false; }

    const auto& d = ParameterRegistry::get (*modTarget);
    const float base = d.clampValue ((float) getValue());
    ring.setBase (d.toNormalised (base));
    ring.setCurrent (d.toNormalised (d.clampValue (base + snapshot.modulation[index])));
    ring.setRange (d.toNormalised (d.clampValue (base + snapshot.modMin[index])),
                   d.toNormalised (d.clampValue (base + snapshot.modMax[index])));
    ring.setSourceCount ((int) snapshot.targeted[index]);
    return true;
}

void AMKnob::setLabel (const juce::String& text) { label = text; labelUpper = text.toUpperCase(); repaint(); }
void AMKnob::setAccent (juce::Colour c) { accent = c; repaint(); }

float AMKnob::proportion() const
{
    const auto range = getRange();
    return range.getLength() > 0.0 ? (float) ((getValue() - range.getStart()) / range.getLength()) : 0.0f;
}

void AMKnob::valueChanged()
{
    ring.setBase (proportion());
}

float AMKnob::labelHeight() const
{
    return layout::knobFootprint ((float) getWidth(), (float) getHeight(), hero, labelUpper.isNotEmpty()).labelHeight;
}

juce::Rectangle<float> AMKnob::knobBounds() const
{
    const auto b = getLocalBounds().toFloat();
    const auto f = layout::knobFootprint (b.getWidth(), b.getHeight(), hero, labelUpper.isNotEmpty());
    return { b.getCentreX() - f.diameter * 0.5f, b.getY() + f.top, f.diameter, f.diameter };
}

void AMKnob::resized()
{
    // The ring paints the outermost orbit of the knob's own footprint, so a modulated
    // knob never grows and never overlaps its value arc.
    ring.setBounds (knobBounds().toNearestInt());
    juce::Slider::resized();
}

void AMKnob::paint (juce::Graphics& g)
{
    const auto kb = knobBounds();
    const float d = kb.getWidth();
    if (d < 4.0f) return;

    const auto geo = rings();
    const float p = proportion();
    const float startAngle = getRotaryParameters().startAngleRadians;
    const float endAngle   = getRotaryParameters().endAngleRadians;
    const float angle = startAngle + p * (endAngle - startAngle);

    const float trackW = geo.trackWidth;
    const auto arcBounds = geo.arc;
    const auto body = geo.body;
    // Hover is taken from where the pointer actually is, not from the eased value
    // alone: a knob that loses its mouse-exit (a page hidden under the pointer, a
    // panel that rebuilt its controls) would otherwise keep showing its value for
    // ever, and the label would never come back.
    const bool pointerOn = dragging || (isShowing() && isMouseOverOrDragging (true));
    if (! pointerOn && hover.target != 0.0f) anim.animate (hover, 0.0f);
    const float reach = pointerOn ? hover.value : 0.0f;

    // Hover lifts the sheen and the ring glow; dragging lifts them a little further.
    const float lit = juce::jmax (reach * 0.72f, dragging ? 1.0f : 0.0f);
    // The label and the value swap places, so their cross-fade has to reach the ends:
    // a partial fade leaves both of them printed on top of each other.
    const float textLit = juce::jmax (reach, dragging ? 1.0f : 0.0f);
    const float glowAmount = juce::jlimit (0.0f, 1.0f, 0.20f + 0.30f * activity + 0.40f * lit);
    const float valueWeight = bipolar ? std::abs (p - 0.5f) * 2.0f : p;
    const auto pair = Theme::accentPair (accent);

    // 1. Outer arc ring — a soft outer glow that grows with the value, then the
    //    dark unfilled track it runs in, then the lit part in the accent pair.
    draw::glowEllipse (g, arcBounds, pair.second, d * 0.14f, glowAmount * (0.18f + 0.82f * valueWeight));

    {
        auto track = draw::arc (arcBounds, startAngle, endAngle);
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.strokePath (track, juce::PathStrokeType (trackW * 1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (Theme::knobTrack.brighter (0.16f));
        g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 2. A dark moat between ring and body, so the ring reads as separate hardware.
    {
        const float moat = (arcBounds.getWidth() - body.getWidth()) * 0.5f;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (body.expanded (juce::jmax (0.0f, moat - trackW * 1.15f)));
    }

    // 3-6. The moulded body: dome, machined rim, specular bloom, contact shadow.
    draw::domeBody (g, body, Theme::knobBase, lit * 0.7f);

    // The lit part of the value arc, over the moat so its glow spills on the metal.
    {
        float from = startAngle, to = angle;
        if (bipolar)
        {
            const float mid = (startAngle + endAngle) * 0.5f;
            from = juce::jmin (mid, angle);
            to   = juce::jmax (mid, angle);

            // centre tick, cut into the track
            g.setColour (Theme::textDim.withAlpha (0.85f));
            g.fillEllipse (arcBounds.getCentreX() - trackW * 0.55f, arcBounds.getY() - trackW * 0.55f, trackW * 1.1f, trackW * 1.1f);
        }

        if (to > from + 0.004f)
        {
            const auto valueArc = draw::arc (arcBounds, from, to);
            // halo first, then the crisp two-stop gradient core
            g.setColour (pair.second.withAlpha (0.10f + 0.16f * glowAmount));
            g.strokePath (valueArc, juce::PathStrokeType (trackW * 3.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (pair.first.withAlpha (0.16f + 0.20f * glowAmount));
            g.strokePath (valueArc, juce::PathStrokeType (trackW * 2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            draw::gradientArc (g, arcBounds, from, to, pair.first, pair.second, trackW, 0.92f + 0.08f * lit);
        }

        // bright tip at the live end of the arc
        const float r = arcBounds.getWidth() * 0.5f;
        const juce::Point<float> tip (arcBounds.getCentreX() + std::sin (angle) * r, arcBounds.getCentreY() - std::cos (angle) * r);
        if (valueWeight > 0.002f || lit > 0.01f)
            draw::glowDot (g, tip, trackW * 0.5f, pair.second.brighter (0.35f), 0.35f + 0.65f * lit);
    }

    // 5. The indicator: the brightest thing on the knob, cut across the dome.
    {
        const float r = body.getWidth() * 0.5f;
        const auto c = body.getCentre();
        const juce::Point<float> inner (c.x + std::sin (angle) * r * 0.50f, c.y - std::cos (angle) * r * 0.50f);
        const juce::Point<float> outer (c.x + std::sin (angle) * r * 0.90f, c.y - std::cos (angle) * r * 0.90f);
        const float w = juce::jmax (1.4f, d * 0.030f);

        // the groove it sits in
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine (inner.x, inner.y + w * 0.6f, outer.x, outer.y + w * 0.6f, w * 1.3f);

        // faint halo, then the crisp near-white core: the brightest thing on the knob
        g.setColour (juce::Colour (0xffe8ecff).withAlpha (0.16f + 0.12f * lit));
        g.drawLine (inner.x, inner.y, outer.x, outer.y, w * 3.0f);
        g.setColour (juce::Colour (0xfff2f4ff).withAlpha (0.30f + 0.16f * lit));
        g.drawLine (inner.x, inner.y, outer.x, outer.y, w * 1.8f);
        g.setColour (juce::Colours::white.withAlpha (0.96f));
        g.drawLine (inner.x, inner.y, outer.x, outer.y, w);
    }

    // Assign mode: every modulatable knob offers itself as a destination.
    if (isAssignTarget())
    {
        const auto halo = geo.orbit;
        g.setColour (Theme::amber.withAlpha (0.16f + 0.14f * lit));
        g.fillEllipse (halo);
        g.setColour (Theme::amber.withAlpha (0.55f + 0.45f * lit));
        g.drawEllipse (halo, juce::jmax (1.0f, geo.ringStroke * 1.1f));
    }

    // Label / value (cross-fades with hover)
    if (labelUpper.isNotEmpty())
    {
        const auto full = getLocalBounds().toFloat();
        const auto labelArea = juce::Rectangle<float> (full.getX(), kb.getBottom() + 2.0f, full.getWidth(), labelHeight());
        const float h = juce::jlimit (8.5f, hero ? 15.0f : 13.0f, labelArea.getHeight() * 0.7f);
        if (textLit > 0.02f)
            draw::trackedText (g, getTextFromValue (getValue()), labelArea, juce::Justification::centred, Theme::valueFont (h + 1.0f),
                               pair.second.brighter (0.25f).withAlpha (textLit));
        if (textLit < 0.98f)
            draw::trackedText (g, labelUpper, labelArea, juce::Justification::centred,
                               draw::fitFont (Theme::labelFont (h), labelUpper, labelArea.getWidth() - 2.0f), Theme::textSecondary.withAlpha (1.0f - textLit));
    }
}

void AMKnob::visibilityChanged()
{
    // A knob hidden under the pointer never gets its mouse exit, so it clears here.
    if (! isShowing()) { hover.snap (0.0f); dragging = false; }
}

void AMKnob::mouseEnter (const juce::MouseEvent& e) { anim.animate (hover, 1.0f); juce::Slider::mouseEnter (e); }
void AMKnob::mouseExit (const juce::MouseEvent& e)  { anim.animate (hover, 0.0f); juce::Slider::mouseExit (e); }

void AMKnob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }
    // Assign mode swallows the click instead of starting a drag.
    if (modTarget.has_value() && ModAssign::get().assignTo (*modTarget, this))
        return;
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 240);
    dragging = true;
    juce::Slider::mouseDown (e);
    repaint();
}

void AMKnob::mouseDrag (const juce::MouseEvent& e)
{
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 240);
    juce::Slider::mouseDrag (e);
}

void AMKnob::mouseUp (const juce::MouseEvent& e)
{
    dragging = false;
    juce::Slider::mouseUp (e);
    repaint();
}

void AMKnob::showContextMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader (labelUpper);
    menu.addItem (1, "Reset to default");
    menu.addItem (2, "Enter value...");
    if (onContextMenu) onContextMenu (menu);

    juce::Component::SafePointer<AMKnob> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int result)
    {
        if (safe == nullptr) return;
        if (result == 1)
        {
            safe->setValue (safe->getDoubleClickReturnValue(), juce::sendNotificationSync);
        }
        else if (result == 2)
        {
            auto* box = new juce::AlertWindow ("Enter value", safe->label, juce::MessageBoxIconType::NoIcon);
            box->addTextEditor ("value", safe->getTextFromValue (safe->getValue()));
            box->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, box] (int r)
            {
                if (r == 1 && safe != nullptr)
                    safe->setValue (safe->getValueFromText (box->getTextEditorContents ("value")), juce::sendNotificationSync);
            }), true);
        }
    });
}

} // namespace am::ui
