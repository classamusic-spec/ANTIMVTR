#include "AMButton.h"

namespace am::ui
{

AMButton::AMButton (const juce::String& text, juce::Colour a)
    : juce::Button (text), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.3f);
    const bool active = highlighted || down || getToggleState();

    if (outlined || active)
    {
        if (active)
            draw::glowRoundedRect (g, b.reduced (1.0f), corner, accent, 10.0f, down ? 0.9f : 0.5f);
        g.setColour (active ? accent.withAlpha (0.12f) : Theme::glass);
        g.fillRoundedRectangle (b.reduced (1.0f), corner);
        g.setColour (active ? accent.withAlpha (0.5f) : Theme::border);
        g.drawRoundedRectangle (b.reduced (1.0f), corner, 1.0f);
    }

    auto textArea = b;
    const float h = juce::jlimit (9.0f, 13.5f, b.getHeight() * 0.36f);
    if (iconGlyph.has_value())
    {
        auto iconArea = textArea.removeFromLeft (b.getHeight()).reduced (b.getHeight() * 0.28f);
        Icons::draw (g, *iconGlyph, iconArea, active ? accent : Theme::textSecondary);
    }
    draw::trackedText (g, getButtonText().toUpperCase(), textArea, juce::Justification::centred, Theme::labelFont (h),
                       active ? Theme::textPrimary : Theme::textSecondary.brighter (0.25f));
}

//==============================================================================
AMIconButton::AMIconButton (Icon i, juce::Colour a) : juce::Button ("icon"), icon (i), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMIconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const bool active = highlighted || down;
    const auto circle = b.withSizeKeepingCentre (juce::jmin (b.getWidth(), b.getHeight()), juce::jmin (b.getWidth(), b.getHeight()));
    if (active)
    {
        draw::glowEllipse (g, circle.reduced (2.0f), accent, 8.0f, 0.6f);
        g.setColour (accent.withAlpha (0.1f));
        g.fillEllipse (circle.reduced (2.0f));
    }
    Icons::draw (g, icon, circle.reduced (circle.getWidth() * 0.26f), active ? Theme::textPrimary : accent);
}

//==============================================================================
AMSegment::AMSegment (juce::StringArray i, juce::Colour a) : items (std::move (i)), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMSegment::setSelected (int index, juce::NotificationType notify)
{
    index = juce::jlimit (0, items.size() - 1, index);
    if (index == selected) return;
    selected = index;
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

int AMSegment::indexAt (juce::Point<int> p) const
{
    if (items.isEmpty()) return -1;
    const int w = getWidth() / items.size();
    return juce::jlimit (0, items.size() - 1, p.x / juce::jmax (1, w));
}

void AMSegment::mouseDown (const juce::MouseEvent& e) { setSelected (indexAt (e.getPosition())); }
void AMSegment::mouseMove (const juce::MouseEvent& e) { const int h = indexAt (e.getPosition()); if (h != hover) { hover = h; repaint(); } }

void AMSegment::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (b.getHeight() * 0.5f, 14.0f);
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (b, corner);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);

    if (items.isEmpty()) return;
    const float w = b.getWidth() / (float) items.size();
    const float h = juce::jlimit (8.5f, 12.5f, b.getHeight() * 0.42f);

    for (int i = 0; i < items.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight());
        const bool on = i == selected;
        if (on)
        {
            auto pill = cell.reduced (2.5f);
            draw::glowRoundedRect (g, pill, corner - 2.0f, accent, 8.0f, 0.55f);
            juce::ColourGradient grad (accent.withAlpha (0.28f), pill.getX(), pill.getY(), accent.withAlpha (0.12f), pill.getX(), pill.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (pill, corner - 2.0f);
            g.setColour (accent.withAlpha (0.45f));
            g.drawRoundedRectangle (pill, corner - 2.0f, 1.0f);
        }
        draw::trackedText (g, items[i].toUpperCase(), cell, juce::Justification::centred, Theme::labelFont (h),
                           on ? Theme::textPrimary : (i == hover ? Theme::textSecondary.brighter (0.4f) : Theme::textSecondary));
    }
}

//==============================================================================
AMSlider::AMSlider (const juce::String& l, juce::Colour a) : label (l), accent (a)
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setWantsKeyboardFocus (false);
    setDoubleClickReturnValue (true, 0.0);
    setRepaintsOnMouseActivity (true);
}

void AMSlider::resized()
{
    juce::Slider::resized();
}

void AMSlider::mouseDown (const juce::MouseEvent& e)
{
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 250);
    juce::Slider::mouseDown (e);
}

void AMSlider::mouseDrag (const juce::MouseEvent& e)
{
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 250);
    juce::Slider::mouseDrag (e);
}

void AMSlider::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float h = juce::jlimit (9.0f, 13.0f, b.getHeight() * 0.42f);
    const float labelW = juce::jmin (b.getWidth() * 0.22f, 90.0f);
    const float valueW = juce::jmin (b.getWidth() * 0.14f, 56.0f);

    auto labelArea = b.removeFromLeft (labelW);
    auto valueArea = b.removeFromRight (valueW);
    auto track = b.reduced (8.0f, 0.0f);

    draw::trackedText (g, label.toUpperCase(), labelArea, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textPrimary.withAlpha (0.85f));
    draw::trackedText (g, getTextFromValue (getValue()), valueArea, juce::Justification::centredRight, Theme::valueFont (h + 1.0f), Theme::textPrimary.withAlpha (0.9f));

    const float y = track.getCentreY();
    const auto range = getRange();
    const float p = range.getLength() > 0.0 ? (float) ((getValue() - range.getStart()) / range.getLength()) : 0.0f;
    const float x = track.getX() + track.getWidth() * p;
    const float lit = isMouseOverOrDragging() ? 1.0f : 0.55f;

    g.setColour (Theme::knobTrack);
    g.drawLine (track.getX(), y, track.getRight(), y, 2.0f);

    juce::Path fill;
    fill.startNewSubPath (track.getX(), y);
    fill.lineTo (x, y);
    draw::glowPath (g, fill, accent, 2.0f, 8.0f, lit);

    const float r = juce::jlimit (5.0f, 9.0f, b.getHeight() * 0.22f);
    draw::glowEllipse (g, juce::Rectangle<float> (x - r, y - r, r * 2, r * 2), accent, r * 1.5f, lit);
    g.setColour (Theme::textPrimary);
    g.fillEllipse (x - r, y - r, r * 2, r * 2);
}

} // namespace am::ui
