#include "AMButton.h"

namespace am::ui
{

AMButton::AMButton (const juce::String& text, juce::Colour a)
    : juce::Button (text), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMButton::mouseEnter (const juce::MouseEvent& e) { anim.animate (hover, 1.0f); juce::Button::mouseEnter (e); }
void AMButton::mouseExit (const juce::MouseEvent& e)  { anim.animate (hover, 0.0f); juce::Button::mouseExit (e); }

void AMButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = chip ? b.getHeight() * 0.5f : juce::jmin (10.0f, b.getHeight() * 0.3f);
    const bool on = getToggleState();
    const float lit = juce::jlimit (0.0f, 1.0f, juce::jmax (hover.value, highlighted ? 0.6f : 0.0f) + (down ? 0.4f : 0.0f));
    const float energy = juce::jlimit (0.0f, 1.0f, lit + (on ? 0.7f : 0.0f));

    if (outlined || filled || chip || on)
    {
        auto r = b.reduced (1.0f);
        if (energy > 0.05f)
            draw::glowRoundedRect (g, r, corner, accent, 9.0f, energy * 0.5f);
        if (filled || on)
        {
            juce::ColourGradient grad (accent.withAlpha (0.30f + 0.15f * lit), r.getX(), r.getY(), accent.withAlpha (0.12f), r.getX(), r.getBottom(), false);
            g.setGradientFill (grad);
        }
        else
        {
            g.setColour (Theme::panelInset.withAlpha (0.9f).brighter (0.05f * lit));
        }
        g.fillRoundedRectangle (r, corner);
        g.setColour ((filled || on) ? accent.withAlpha (0.55f) : Theme::border.withAlpha (0.08f + 0.2f * lit));
        g.drawRoundedRectangle (r, corner, 1.0f);
    }
    else if (lit > 0.02f)
    {
        // text button: soft accent underline glow on hover
        juce::Path line;
        line.startNewSubPath (b.getX() + b.getWidth() * 0.2f, b.getBottom() - 4.0f);
        line.lineTo (b.getRight() - b.getWidth() * 0.2f, b.getBottom() - 4.0f);
        draw::glowPath (g, line, accent.withAlpha (lit), 1.0f, 6.0f, lit * 0.8f);
    }

    auto textArea = b;
    const float h = chip ? juce::jlimit (8.0f, 11.0f, b.getHeight() * 0.42f) : juce::jlimit (9.0f, 13.0f, b.getHeight() * 0.34f);
    const auto textColour = (on || filled) ? Theme::textPrimary
                                           : Theme::textSecondary.interpolatedWith (Theme::textPrimary, 0.25f + 0.75f * lit);
    if (iconGlyph.has_value())
    {
        auto iconArea = textArea.removeFromLeft (b.getHeight()).reduced (b.getHeight() * 0.3f);
        Icons::draw (g, *iconGlyph, iconArea, on || lit > 0.5f ? accent : Theme::textSecondary, 0.9f);
        textArea = textArea.withTrimmedRight (b.getHeight() * 0.4f);
    }
    draw::trackedText (g, getButtonText().toUpperCase(), textArea, juce::Justification::centred,
                       (on || filled) ? Theme::labelFontStrong (h) : Theme::labelFont (h), textColour);
}

//==============================================================================
AMIconButton::AMIconButton (Icon i, juce::Colour a) : juce::Button ("icon"), icon (i), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMIconButton::mouseEnter (const juce::MouseEvent& e) { anim.animate (hover, 1.0f); juce::Button::mouseEnter (e); }
void AMIconButton::mouseExit (const juce::MouseEvent& e)  { anim.animate (hover, 0.0f); juce::Button::mouseExit (e); }

void AMIconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const float lit = juce::jlimit (0.0f, 1.0f, juce::jmax (hover.value, highlighted ? 0.5f : 0.0f) + (down ? 0.5f : 0.0f));
    const float s = juce::jmin (b.getWidth(), b.getHeight());
    const auto circle = b.withSizeKeepingCentre (s, s);
    if (outlined)
    {
        g.setColour (Theme::border.withAlpha (0.08f + 0.1f * lit));
        g.drawEllipse (circle.reduced (1.5f), 1.0f);
    }
    if (lit > 0.02f)
    {
        draw::glowEllipse (g, circle.reduced (2.0f), accent, 8.0f, 0.6f * lit);
        g.setColour (accent.withAlpha (0.12f * lit));
        g.fillEllipse (circle.reduced (2.0f));
    }
    Icons::draw (g, icon, circle.reduced (s * 0.28f), accent.interpolatedWith (Theme::textPrimary, lit), 0.9f);
}

//==============================================================================
AMSegment::AMSegment (juce::StringArray i, juce::Colour a) : items (std::move (i)), accent (a)
{
    setWantsKeyboardFocus (false);
    thumb.snap (0.0f);
}

void AMSegment::setSelected (int index, juce::NotificationType notify)
{
    index = juce::jlimit (0, items.size() - 1, index);
    if (index == selected) return;
    selected = index;
    if (isShowing()) anim.animate (thumb, (float) selected); else thumb.snap ((float) selected);
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
    juce::ColourGradient shade (juce::Colours::black.withAlpha (0.35f), 0.0f, b.getY(), juce::Colours::transparentBlack, 0.0f, b.getCentreY(), false);
    g.setGradientFill (shade);
    g.fillRoundedRectangle (b, corner);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);

    if (items.isEmpty()) return;
    const float w = b.getWidth() / (float) items.size();
    const float h = juce::jlimit (8.0f, 12.0f, b.getHeight() * 0.4f);

    // gliding thumb
    {
        auto pill = juce::Rectangle<float> (b.getX() + w * thumb.value, b.getY(), w, b.getHeight()).reduced (2.5f);
        draw::glowRoundedRect (g, pill, corner - 2.0f, accent, 8.0f, 0.5f);
        juce::ColourGradient grad (accent.withAlpha (0.30f), pill.getX(), pill.getY(), accent.withAlpha (0.10f), pill.getX(), pill.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (pill, corner - 2.0f);
        g.setColour (accent.withAlpha (0.5f));
        g.drawRoundedRectangle (pill, corner - 2.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine (pill.getX() + corner * 0.6f, pill.getY() + 1.0f, pill.getRight() - corner * 0.6f, pill.getY() + 1.0f, 1.0f);
    }

    for (int i = 0; i < items.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight());
        const bool on = i == selected;
        draw::trackedText (g, items[i].toUpperCase(), cell, juce::Justification::centred, on ? Theme::labelFontStrong (h) : Theme::labelFont (h),
                           on ? Theme::textPrimary : (i == hover ? Theme::textSecondary.brighter (0.4f) : Theme::textSecondary));
    }
}

//==============================================================================
AMSlider::AMSlider (const juce::String& l, juce::Colour a) : label (l.toUpperCase()), accent (a)
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setWantsKeyboardFocus (false);
    setDoubleClickReturnValue (true, 0.0);
    setRepaintsOnMouseActivity (false);
}

void AMSlider::mouseEnter (const juce::MouseEvent& e) { anim.animate (hover, 1.0f); juce::Slider::mouseEnter (e); }
void AMSlider::mouseExit (const juce::MouseEvent& e)  { anim.animate (hover, 0.0f); juce::Slider::mouseExit (e); }

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
    const float h = juce::jlimit (8.5f, 12.5f, b.getHeight() * 0.4f);
    const float labelW = showLabel ? juce::jmin (b.getWidth() * 0.24f, 92.0f) : 0.0f;
    const float valueW = showValue ? juce::jmin (b.getWidth() * 0.16f, 58.0f) : 0.0f;
    const float lit = juce::jmax (hover.value, isMouseButtonDown() ? 1.0f : 0.0f);

    auto labelArea = b.removeFromLeft (labelW);
    auto valueArea = b.removeFromRight (valueW);
    const float r = juce::jlimit (4.5f, 8.0f, b.getHeight() * 0.2f);
    auto track = b.reduced (r + 4.0f, 0.0f);

    if (showLabel)
        draw::trackedText (g, label, labelArea, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textPrimary.withAlpha (0.82f));
    if (showValue)
        draw::trackedText (g, getTextFromValue (getValue()), valueArea, juce::Justification::centredRight, Theme::valueFont (h + 1.0f),
                           Theme::textValue.interpolatedWith (accent, 0.5f * lit));

    const float y = track.getCentreY();
    const auto range = getRange();
    const float p = range.getLength() > 0.0 ? (float) ((getValue() - range.getStart()) / range.getLength()) : 0.0f;
    const float x = track.getX() + track.getWidth() * p;

    // track groove
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (track.getX(), y + 1.0f, track.getRight(), y + 1.0f, 2.0f);
    g.setColour (Theme::knobTrack);
    g.drawLine (track.getX(), y, track.getRight(), y, 2.0f);

    // lit portion
    if (p > 0.002f)
    {
        juce::Path fill;
        fill.startNewSubPath (track.getX(), y);
        fill.lineTo (x, y);
        draw::glowPath (g, fill, accent, 2.0f, 9.0f, 0.45f + 0.55f * lit);
    }

    // white thumb with a soft accent halo
    draw::glowEllipse (g, juce::Rectangle<float> (x - r, y - r, r * 2, r * 2), accent, r * 1.6f, 0.5f + 0.5f * lit);
    juce::ColourGradient thumb (juce::Colours::white, x - r * 0.3f, y - r * 0.4f, juce::Colour (0xffc8c8d4), x + r, y + r, true);
    g.setGradientFill (thumb);
    g.fillEllipse (x - r, y - r, r * 2, r * 2);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (x - r, y - r, r * 2, r * 2, 1.0f);
}

} // namespace am::ui
