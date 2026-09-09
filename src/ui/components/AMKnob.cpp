#include "AMKnob.h"

namespace am::ui
{

AMKnob::AMKnob (const juce::String& labelText, juce::Colour accentColour)
    : label (labelText), accent (accentColour)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setMouseDragSensitivity (240);
    setVelocityBasedMode (false);
    setDoubleClickReturnValue (true, 0.0);
    setScrollWheelEnabled (true);
    setWantsKeyboardFocus (false);
    setRepaintsOnMouseActivity (true);
}

juce::Rectangle<float> AMKnob::knobBounds() const
{
    auto b = getLocalBounds().toFloat();
    const float labelH = label.isNotEmpty() ? juce::jlimit (10.0f, 22.0f, b.getHeight() * 0.18f) : 0.0f;
    auto area = b.withTrimmedBottom (labelH + 2.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight()) * 0.92f;
    return area.withSizeKeepingCentre (d, d);
}

void AMKnob::paint (juce::Graphics& g)
{
    const auto kb = knobBounds();
    const float d = kb.getWidth();
    if (d < 4.0f) return;

    const auto range = getRange();
    const double proportion = range.getLength() > 0.0 ? (getValue() - range.getStart()) / range.getLength() : 0.0;
    const float startAngle = getRotaryParameters().startAngleRadians;
    const float endAngle   = getRotaryParameters().endAngleRadians;
    const float angle = startAngle + (float) proportion * (endAngle - startAngle);

    const float trackW = juce::jmax (1.2f, d * 0.045f);
    const auto arcBounds = kb.reduced (trackW * 1.2f);
    const bool lit = hovering || dragging;
    const float glowAmount = juce::jlimit (0.0f, 1.0f, 0.35f + 0.4f * activity + (lit ? 0.35f : 0.0f));

    // Glow (kept small and controlled)
    draw::glowEllipse (g, arcBounds, accent, d * 0.16f, glowAmount * (float) (0.25 + 0.75 * proportion));

    // Base
    {
        juce::ColourGradient base (Theme::knobBase.brighter (0.25f), kb.getX(), kb.getY(), Theme::knobBase.darker (0.35f), kb.getX(), kb.getBottom(), false);
        g.setGradientFill (base);
        g.fillEllipse (kb.reduced (trackW * 2.6f));
        g.setColour (Theme::border);
        g.drawEllipse (kb.reduced (trackW * 2.6f), 1.0f);
    }

    // Track
    g.setColour (Theme::knobTrack);
    g.strokePath (draw::arc (arcBounds, startAngle, endAngle), juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    {
        juce::Path arc;
        if (bipolar)
        {
            const float mid = (startAngle + endAngle) * 0.5f;
            arc = draw::arc (arcBounds, juce::jmin (mid, angle), juce::jmax (mid, angle));
        }
        else
        {
            arc = draw::arc (arcBounds, startAngle, angle);
        }
        if (! arc.isEmpty())
            draw::glowPath (g, arc, accent, trackW, trackW * 3.0f, glowAmount);
    }

    // Indicator
    {
        const float r = kb.getWidth() * 0.5f - trackW * 2.6f;
        const auto c = kb.getCentre();
        juce::Point<float> inner (c.x + std::sin (angle) * r * 0.55f, c.y - std::cos (angle) * r * 0.55f);
        juce::Point<float> outer (c.x + std::sin (angle) * r * 0.92f, c.y - std::cos (angle) * r * 0.92f);
        g.setColour (Theme::textPrimary.withAlpha (lit ? 1.0f : 0.85f));
        g.drawLine (juce::Line<float> (inner, outer), juce::jmax (1.2f, d * 0.035f));
    }

    // Label / value
    if (label.isNotEmpty())
    {
        const auto labelArea = getLocalBounds().toFloat().withTop (kb.getBottom() + 2.0f);
        const float h = juce::jlimit (8.5f, 14.0f, labelArea.getHeight() * 0.72f);
        if (lit)
            draw::trackedText (g, getTextFromValue (getValue()), labelArea, juce::Justification::centred, Theme::valueFont (h + 1.0f), accent.brighter (0.2f));
        else
            draw::trackedText (g, label.toUpperCase(), labelArea, juce::Justification::centred, Theme::labelFont (h), Theme::textSecondary);
    }
}

void AMKnob::mouseEnter (const juce::MouseEvent& e) { hovering = true; juce::Slider::mouseEnter (e); repaint(); }
void AMKnob::mouseExit (const juce::MouseEvent& e)  { hovering = false; juce::Slider::mouseExit (e); repaint(); }

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
