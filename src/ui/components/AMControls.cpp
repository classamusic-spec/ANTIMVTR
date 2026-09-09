#include "AMControls.h"

namespace am::ui
{

AMToggle::AMToggle (const juce::String& l, juce::Colour a) : label (l.toUpperCase()), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMToggle::setToggleState (bool on, juce::NotificationType notify)
{
    if (on == state) return;
    state = on;
    if (isShowing()) anim.animate (lit, on ? 1.0f : 0.0f); else lit.snap (on ? 1.0f : 0.0f);
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (state);
}

void AMToggle::mouseDown (const juce::MouseEvent&) { setToggleState (! state); }

void AMToggle::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool hasLabel = label.isNotEmpty() && b.getHeight() > 30.0f;
    const float labelH = hasLabel ? juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.32f) : 0.0f;
    auto labelArea = b.removeFromBottom (labelH);
    const float pillH = juce::jlimit (14.0f, 24.0f, b.getHeight() * 0.7f);
    const float pillW = juce::jmin (b.getWidth() - 4.0f, pillH * 2.1f);
    auto pill = b.withSizeKeepingCentre (pillW, pillH);
    const float corner = pillH * 0.5f;
    const float on = lit.value;

    if (on > 0.02f) draw::glowRoundedRect (g, pill, corner, accent, 8.0f, 0.5f * on);
    g.setColour (Theme::panelInset.interpolatedWith (accent.withAlpha (0.25f), on));
    g.fillRoundedRectangle (pill, corner);
    g.setColour (Theme::border.interpolatedWith (accent.withAlpha (0.6f), on).withMultipliedAlpha (1.0f + 0.5f * hover.value));
    g.drawRoundedRectangle (pill.reduced (0.5f), corner, 1.0f);

    // knob travels left → right
    const float kr = pillH * 0.5f - 3.0f;
    const float kx = pill.getX() + 3.0f + kr + (pill.getWidth() - 6.0f - kr * 2.0f) * on;
    const auto knob = juce::Rectangle<float> (kx - kr, pill.getCentreY() - kr, kr * 2.0f, kr * 2.0f);
    if (on > 0.02f) draw::glowEllipse (g, knob, accent, kr * 1.2f, on);
    g.setColour (Theme::textSecondary.interpolatedWith (juce::Colours::white, on));
    g.fillEllipse (knob);

    if (hasLabel)
    {
        const float h = juce::jlimit (8.0f, 11.5f, labelH * 0.68f);
        draw::trackedText (g, label, labelArea, juce::Justification::centred, Theme::labelFont (h),
                           Theme::textSecondary.interpolatedWith (Theme::textPrimary, 0.5f * on + 0.3f * hover.value));
    }
}

//==============================================================================
AMChoice::AMChoice (const juce::String& l, juce::StringArray c, juce::Colour a)
    : label (l.toUpperCase()), choices (std::move (c)), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMChoice::setSelected (int index, juce::NotificationType notify)
{
    if (choices.isEmpty()) return;
    index = ((index % choices.size()) + choices.size()) % choices.size();
    if (index == selected) return;
    selected = index;
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

juce::Rectangle<float> AMChoice::pillBounds() const
{
    auto b = getLocalBounds().toFloat();
    const bool hasLabel = showLabel && label.isNotEmpty() && b.getHeight() > 30.0f;
    if (hasLabel) b.removeFromTop (juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.3f));
    const float pillH = juce::jlimit (18.0f, 30.0f, b.getHeight() * 0.72f);
    return b.withSizeKeepingCentre (b.getWidth() - 2.0f, pillH);
}

void AMChoice::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) > 0.01f) setSelected (selected + (wheel.deltaY > 0 ? -1 : 1));
}

void AMChoice::mouseDown (const juce::MouseEvent& e)
{
    const auto pill = pillBounds();
    const float zone = juce::jmin (pill.getWidth() * 0.28f, pill.getHeight() * 1.2f);
    if (e.position.x < pill.getX() + zone)       { setSelected (selected - 1); return; }
    if (e.position.x > pill.getRight() - zone)   { setSelected (selected + 1); return; }

    juce::PopupMenu menu;
    if (label.isNotEmpty()) menu.addSectionHeader (label);
    for (int i = 0; i < choices.size(); ++i) menu.addItem (i + 1, choices[i], true, i == selected);
    juce::Component::SafePointer<AMChoice> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()), [safe] (int r)
    {
        if (safe != nullptr && r > 0) safe->setSelected (r - 1);
    });
}

void AMChoice::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool hasLabel = showLabel && label.isNotEmpty() && b.getHeight() > 30.0f;
    if (hasLabel)
    {
        auto labelArea = b.removeFromTop (juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.3f));
        const float h = juce::jlimit (8.0f, 11.0f, labelArea.getHeight() * 0.68f);
        draw::trackedText (g, label, labelArea, juce::Justification::centred, Theme::labelFont (h), Theme::textSecondary);
    }
    const auto pill = pillBounds();
    const float corner = pill.getHeight() * 0.5f;
    const float lit = hover.value;

    if (lit > 0.02f) draw::glowRoundedRect (g, pill, corner, accent, 7.0f, 0.3f * lit);
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (pill, corner);
    g.setColour (Theme::border.withMultipliedAlpha (1.0f + lit));
    g.drawRoundedRectangle (pill.reduced (0.5f), corner, 1.0f);

    const float zone = juce::jmin (pill.getWidth() * 0.28f, pill.getHeight() * 1.2f);
    const auto chevColour = Theme::textSecondary.interpolatedWith (accent, lit);
    draw::chevron (g, pill.withWidth (zone).reduced (zone * 0.25f, pill.getHeight() * 0.28f), -1, chevColour);
    draw::chevron (g, pill.withLeft (pill.getRight() - zone).reduced (zone * 0.25f, pill.getHeight() * 0.28f), 1, chevColour);

    const float h = juce::jlimit (8.5f, 12.0f, pill.getHeight() * 0.42f);
    const juce::String text = choices.isEmpty() ? juce::String() : choices[selected];
    draw::trackedText (g, text, pill.reduced (zone, 0.0f), juce::Justification::centred, Theme::labelFont (h), Theme::textPrimary);
}

} // namespace am::ui
