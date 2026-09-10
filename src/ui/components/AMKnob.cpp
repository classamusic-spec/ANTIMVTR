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
    return labelUpper.isNotEmpty() ? juce::jlimit (10.0f, 22.0f, (float) getHeight() * 0.19f) : 0.0f;
}

juce::Rectangle<float> AMKnob::knobBounds() const
{
    auto b = getLocalBounds().toFloat();
    const float labelH = labelHeight();
    const float cap = hero ? 150.0f : 100.0f;
    const float d = juce::jmin (b.getWidth(), b.getHeight() - labelH - 2.0f, cap) * 0.92f;
    const float groupH = d + (labelH > 0.0f ? labelH + 2.0f : 0.0f);
    const float top = b.getY() + juce::jmax (0.0f, (b.getHeight() - groupH) * 0.5f);
    return { b.getCentreX() - d * 0.5f, top, d, d };
}

void AMKnob::resized()
{
    const auto kb = knobBounds();
    const float d = kb.getWidth();
    const float trackW = juce::jmax (1.4f, d * (hero ? 0.03f : 0.026f));
    ring.setBounds (kb.reduced (trackW * 1.1f).expanded (trackW * 2.2f).toNearestInt());
    juce::Slider::resized();
}

void AMKnob::paint (juce::Graphics& g)
{
    const auto kb = knobBounds();
    const float d = kb.getWidth();
    if (d < 4.0f) return;

    const float p = proportion();
    const float startAngle = getRotaryParameters().startAngleRadians;
    const float endAngle   = getRotaryParameters().endAngleRadians;
    const float angle = startAngle + p * (endAngle - startAngle);

    const float trackW = juce::jmax (1.4f, d * (hero ? 0.03f : 0.026f));
    const auto arcBounds = kb.reduced (trackW * 1.1f);
    const float lit = juce::jmax (hover.value, dragging ? 1.0f : 0.0f);
    const float glowAmount = juce::jlimit (0.0f, 1.0f, 0.22f + 0.35f * activity + 0.45f * lit);
    const float valueWeight = bipolar ? std::abs (p - 0.5f) * 2.0f : p;

    // Controlled glow behind the arc, scaled by the value so idle knobs stay quiet.
    draw::glowEllipse (g, arcBounds, accent, d * 0.14f, glowAmount * (0.2f + 0.8f * valueWeight));

    // Sphere base
    const auto body = kb.reduced (trackW * 3.6f);
    draw::sphere (g, body, lit * 0.6f);

    // Track
    g.setColour (Theme::knobTrack);
    g.strokePath (draw::arc (arcBounds, startAngle, endAngle), juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    {
        juce::Path valueArc;
        if (bipolar)
        {
            const float mid = (startAngle + endAngle) * 0.5f;
            if (std::abs (angle - mid) > 0.01f) valueArc = draw::arc (arcBounds, juce::jmin (mid, angle), juce::jmax (mid, angle));
            // centre tick
            g.setColour (Theme::textDim);
            g.fillEllipse (arcBounds.getCentreX() - trackW * 0.6f, arcBounds.getY() - trackW * 0.6f, trackW * 1.2f, trackW * 1.2f);
        }
        else if (p > 0.002f)
        {
            valueArc = draw::arc (arcBounds, startAngle, angle);
        }
        if (! valueArc.isEmpty())
            draw::glowPath (g, valueArc, accent, trackW, trackW * 3.2f, glowAmount);

        // bright tip at the end of the arc
        const float r = arcBounds.getWidth() * 0.5f;
        const juce::Point<float> tip (arcBounds.getCentreX() + std::sin (angle) * r, arcBounds.getCentreY() - std::cos (angle) * r);
        if (valueWeight > 0.002f || lit > 0.01f)
            draw::glowDot (g, tip, trackW * 0.55f, accent.brighter (0.3f), 0.4f + 0.6f * lit);
    }

    // Fine white indicator on the sphere
    {
        const float r = body.getWidth() * 0.5f;
        const auto c = body.getCentre();
        const juce::Point<float> inner (c.x + std::sin (angle) * r * 0.58f, c.y - std::cos (angle) * r * 0.58f);
        const juce::Point<float> outer (c.x + std::sin (angle) * r * 0.90f, c.y - std::cos (angle) * r * 0.90f);
        const float w = juce::jmax (1.2f, d * 0.024f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (inner, outer).withShortenedStart (-0.5f), w + 1.5f);
        g.setColour (Theme::textPrimary.withAlpha (0.78f + 0.22f * lit));
        g.drawLine (juce::Line<float> (inner, outer), w);
    }

    // Label / value (cross-fades with hover)
    if (labelUpper.isNotEmpty())
    {
        const auto full = getLocalBounds().toFloat();
        const auto labelArea = juce::Rectangle<float> (full.getX(), kb.getBottom() + 2.0f, full.getWidth(), labelHeight());
        const float h = juce::jlimit (8.5f, hero ? 15.0f : 13.0f, labelArea.getHeight() * 0.7f);
        if (lit > 0.02f)
            draw::trackedText (g, getTextFromValue (getValue()), labelArea, juce::Justification::centred, Theme::valueFont (h + 1.0f),
                               accent.brighter (0.25f).withAlpha (lit));
        if (lit < 0.98f)
            draw::trackedText (g, labelUpper, labelArea, juce::Justification::centred,
                               draw::fitFont (Theme::labelFont (h), labelUpper, labelArea.getWidth() - 2.0f), Theme::textSecondary.withAlpha (1.0f - lit));
    }
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
