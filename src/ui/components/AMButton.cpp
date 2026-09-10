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
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    const float corner = chip ? b.getHeight() * 0.5f : juce::jmin (10.0f, b.getHeight() * 0.32f);
    const bool on = getToggleState();
    const float lit = juce::jlimit (0.0f, 1.0f, juce::jmax (hover.value, highlighted ? 0.6f : 0.0f) + (down ? 0.4f : 0.0f));
    const float energy = juce::jlimit (0.0f, 1.0f, lit + (on ? 0.7f : 0.0f));
    const auto pair = Theme::accentPair (accent);

    // Every button is a small machined key: raised when it is up, pressed into
    // its seat when it is held, lit in the accent when it is on.
    if (energy > 0.05f)
        draw::glowRoundedRect (g, b, corner, pair.second, 9.0f, energy * 0.45f);

    if (down)
    {
        draw::insetWell (g, b, corner, Theme::panelInset, 0.85f);
    }
    else
    {
        draw::SlabStyle style;
        style.top    = Theme::panelTop.brighter (0.05f + 0.10f * lit);
        style.bottom = Theme::panel.darker (0.15f);
        style.shadow = 0.7f;
        style.brush  = 0.7f;
        draw::raisedSlab (g, b, corner, style);
    }

    if (filled || on)
    {
        draw::gradientCapsule (g, b.reduced (1.0f), corner - 1.0f, pair.first, pair.second, 0.26f + 0.12f * lit);
        g.setColour (pair.second.withAlpha (0.55f));
        g.drawRoundedRectangle (b.reduced (1.0f), corner - 1.0f, 1.0f);
    }
    else if (outlined)
    {
        g.setColour (Theme::border.withMultipliedAlpha (1.0f + 1.5f * lit));
        g.drawRoundedRectangle (b.reduced (1.0f), corner - 1.0f, 1.0f);
    }

    auto textArea = b;
    const float h = chip ? juce::jlimit (8.0f, 11.0f, b.getHeight() * 0.42f) : juce::jlimit (9.0f, 13.0f, b.getHeight() * 0.34f);
    const auto textColour = (on || filled) ? Theme::textPrimary
                                           : Theme::textSecondary.interpolatedWith (Theme::textPrimary, 0.3f + 0.7f * lit);
    if (iconGlyph.has_value())
    {
        auto iconArea = textArea.removeFromLeft (b.getHeight()).reduced (b.getHeight() * 0.3f);
        Icons::draw (g, *iconGlyph, iconArea, on || lit > 0.5f ? pair.second : Theme::textSecondary, 0.9f);
        textArea = textArea.withTrimmedRight (b.getHeight() * 0.4f);
    }
    if (down) textArea = textArea.translated (0.0f, 0.7f);

    // The lettering is cut into the key, so it keeps its lit lower lip.
    const auto font = draw::fitFont ((on || filled) ? Theme::labelFontStrong (h) : Theme::labelFont (h),
                                     getButtonText().toUpperCase(), textArea.getWidth() - 4.0f);
    const float trailing = font.getExtraKerningFactor() * font.getHeight() * 0.5f;
    draw::trackedText (g, getButtonText().toUpperCase(), textArea.translated (0.0f, 1.0f), juce::Justification::centred, font,
                       juce::Colours::black.withAlpha (0.55f));
    draw::trackedText (g, getButtonText().toUpperCase(), textArea, juce::Justification::centred, font, textColour);
    juce::ignoreUnused (trailing);
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
    const auto pair = Theme::accentPair (accent);

    if (outlined)
    {
        // A small round key seated in the panel.
        if (down)
        {
            draw::insetWell (g, circle.reduced (1.0f), s * 0.5f, Theme::panelInset, 0.9f);
        }
        else
        {
            draw::domeBody (g, circle.reduced (1.0f), Theme::knobBase.darker (0.15f), lit * 0.7f, 0.8f);
        }
    }
    if (lit > 0.02f)
    {
        draw::glowEllipse (g, circle.reduced (2.0f), pair.second, 8.0f, 0.55f * lit);
        if (! outlined)
        {
            g.setColour (pair.second.withAlpha (0.10f * lit));
            g.fillEllipse (circle.reduced (2.0f));
        }
    }
    auto iconArea = circle.reduced (s * (outlined ? 0.32f : 0.28f));
    if (down) iconArea = iconArea.translated (0.0f, 0.6f);
    Icons::draw (g, icon, iconArea, accent.interpolatedWith (Theme::textPrimary, 0.35f + 0.65f * lit), 0.95f);
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

    // The strip itself is a capsule cut into the panel.
    draw::insetWell (g, b, corner, Theme::panelInset, 1.0f);

    if (items.isEmpty()) return;
    const float w = b.getWidth() / (float) items.size();
    const float h = juce::jlimit (8.0f, 12.0f, b.getHeight() * 0.4f);
    const auto pair = Theme::accentPair (accent);

    // The selected cell is a raised slab that glides between the cells.
    {
        auto pill = juce::Rectangle<float> (b.getX() + w * thumb.value, b.getY(), w, b.getHeight()).reduced (2.5f);
        const float pc = juce::jmax (1.0f, corner - 2.0f);
        draw::glowRoundedRect (g, pill, pc, pair.second, 8.0f, 0.45f);

        draw::SlabStyle style;
        style.top    = Theme::panelTop.brighter (0.16f);
        style.bottom = Theme::panel;
        style.shadow = 0.55f;
        style.brush  = 0.6f;
        draw::raisedSlab (g, pill, pc, style);

        draw::gradientCapsule (g, pill.reduced (0.8f), pc - 0.8f, pair.first, pair.second, 0.30f);
        g.setColour (pair.second.withAlpha (0.55f));
        g.drawRoundedRectangle (pill.reduced (0.8f), pc - 0.8f, 1.0f);
    }

    // Fine seams between the cells, cut into the strip.
    for (int i = 1; i < items.size(); ++i)
    {
        const float x = b.getX() + w * (float) i;
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (x, b.getY() + corner * 0.5f, x, b.getBottom() - corner * 0.5f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.drawLine (x + 1.0f, b.getY() + corner * 0.5f, x + 1.0f, b.getBottom() - corner * 0.5f, 1.0f);
    }

    for (int i = 0; i < items.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight());
        const bool on = i == selected;
        const auto font = draw::fitFont (on ? Theme::labelFontStrong (h) : Theme::labelFont (h), items[i].toUpperCase(), cell.getWidth() - 6.0f);
        draw::trackedText (g, items[i].toUpperCase(), cell.translated (0.0f, 1.0f), juce::Justification::centred, font,
                           juce::Colours::black.withAlpha (0.5f));
        draw::trackedText (g, items[i].toUpperCase(), cell, juce::Justification::centred, font,
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
    const auto pair = Theme::accentPair (accent);

    auto labelArea = b.removeFromLeft (labelW);
    auto valueArea = b.removeFromRight (valueW);
    const float r = juce::jlimit (4.5f, 9.0f, b.getHeight() * 0.24f);
    auto row = b.reduced (r + 3.0f, 0.0f);

    if (showLabel)
        draw::trackedText (g, label, labelArea, juce::Justification::centredLeft, Theme::labelFont (h), Theme::textPrimary.withAlpha (0.82f));
    if (showValue)
        draw::trackedText (g, getTextFromValue (getValue()), valueArea, juce::Justification::centredRight, Theme::valueFont (h + 1.0f),
                           Theme::textValue.interpolatedWith (pair.second, 0.5f * lit).withAlpha (0.85f));

    const float y = row.getCentreY();
    const auto range = getRange();
    const float p = range.getLength() > 0.0 ? (float) ((getValue() - range.getStart()) / range.getLength()) : 0.0f;
    const float x = row.getX() + row.getWidth() * p;

    // Recessed capsule track: dark inside, with a lit lower lip.
    const float trackH = juce::jlimit (4.0f, 11.0f, r * 1.15f);
    const auto track = juce::Rectangle<float> (row.getX() - trackH * 0.5f, y - trackH * 0.5f, row.getWidth() + trackH, trackH);
    draw::insetWell (g, track, trackH * 0.5f, Theme::panelInset, 1.0f);

    // The filled portion, in the section's accent pair, glowing softly.
    {
        const float origin = bipolar ? track.getCentreX() : track.getX() + trackH * 0.5f;
        const float lo = juce::jmin (origin, x), hi = juce::jmax (origin, x);
        if (hi - lo > 0.8f)
        {
            auto fill = juce::Rectangle<float> (lo, y - trackH * 0.5f, hi - lo, trackH).reduced (0.0f, 1.2f).expanded (trackH * 0.42f, 0.0f);
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addRoundedRectangle (track.reduced (1.0f), (trackH - 2.0f) * 0.5f);
            g.reduceClipRegion (clip);
            draw::glowRoundedRect (g, fill, fill.getHeight() * 0.5f, pair.second, 7.0f, 0.35f + 0.45f * lit);
            draw::gradientCapsule (g, fill, fill.getHeight() * 0.5f, pair.first, pair.second, 0.92f);
            // a lit top edge on the fill so it reads as liquid in a groove
            g.setColour (juce::Colours::white.withAlpha (0.20f));
            g.drawLine (lo + 1.0f, y - trackH * 0.5f + 1.6f, hi - 1.0f, y - trackH * 0.5f + 1.6f, 1.0f);
        }
        if (bipolar)
        {
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.fillRect (origin - 0.6f, y - trackH * 0.5f, 1.2f, trackH);
        }
    }

    // The handle: a miniature knob body.
    const auto handle = juce::Rectangle<float> (x - r, y - r, r * 2.0f, r * 2.0f);
    draw::glowEllipse (g, handle, pair.second, r * 1.3f, 0.35f + 0.5f * lit);
    draw::domeBody (g, handle, juce::Colour (0xff9aa0ae).darker (0.15f - 0.1f * lit), 0.5f + 0.5f * lit, 0.9f);
}

} // namespace am::ui
