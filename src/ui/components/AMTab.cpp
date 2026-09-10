#include "AMTab.h"

namespace am::ui
{

AMTab::AMTab (const juce::String& n, std::optional<Icon> i, juce::Colour a)
    : name (n), upper (n.toUpperCase()), icon (i), accent (a)
{
    setWantsKeyboardFocus (false);
}

void AMTab::setSelected (bool on)
{
    if (on == selected) return;
    selected = on;
    if (isShowing()) anim.animate (lit, on ? 1.0f : 0.0f); else lit.snap (on ? 1.0f : 0.0f);
    repaint();
}

void AMTab::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float on = lit.value;
    const float hv = hover.value * (1.0f - on);
    const auto col = Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.6f));

    if (on > 0.02f)
    {
        juce::ColourGradient grad (accent.withAlpha (0.14f * on), b.getX(), b.getBottom(), accent.withAlpha (0.0f), b.getX(), b.getY() + b.getHeight() * 0.2f, false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 4.0f);
        juce::Path line;
        const float inset = style == Style::Nav ? b.getWidth() * 0.18f : b.getWidth() * 0.12f;
        line.startNewSubPath (b.getX() + inset, b.getBottom() - 1.5f);
        line.lineTo (b.getRight() - inset, b.getBottom() - 1.5f);
        draw::glowPath (g, line, accent.withAlpha (on), 1.5f, 8.0f, 0.9f * on);
    }
    else if (hv > 0.02f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f * hv));
        g.fillRoundedRectangle (b, 4.0f);
    }

    if (style == Style::Nav && icon.has_value() && b.getHeight() > 30.0f)
    {
        const float labelH = juce::jlimit (8.5f, 11.0f, b.getHeight() * 0.16f);
        auto iconArea = b.withTrimmedBottom (labelH * 2.4f).reduced (0.0f, b.getHeight() * 0.2f);
        const float d = juce::jmin (iconArea.getWidth(), iconArea.getHeight(), 18.0f);
        iconArea = iconArea.withSizeKeepingCentre (d, d);
        if (on > 0.02f) draw::glowEllipse (g, iconArea, accent, d * 0.6f, 0.5f * on);
        Icons::draw (g, *icon, iconArea, col, 0.75f);
        draw::trackedText (g, upper, b.withTop (iconArea.getBottom() + 4.0f), juce::Justification::centredTop,
                           on > 0.5f ? Theme::labelFontStrong (labelH) : Theme::labelFont (labelH), col);
    }
    else
    {
        const float h = juce::jlimit (8.5f, 12.0f, b.getHeight() * 0.36f);
        draw::trackedText (g, upper, b, juce::Justification::centred, on > 0.5f ? Theme::labelFontStrong (h) : Theme::labelFont (h), col);
    }
}

//==============================================================================
AMTabBar::AMTabBar (juce::StringArray names, juce::Colour accent, std::vector<Icon> icons)
{
    for (int i = 0; i < names.size(); ++i)
    {
        auto t = std::make_unique<AMTab> (names[i], i < (int) icons.size() ? std::optional<Icon> (icons[(size_t) i]) : std::nullopt, accent);
        t->setStyle (style);
        t->onClick = [this, i] { setSelected (i); };
        addAndMakeVisible (*t);
        tabs.push_back (std::move (t));
    }
    if (! tabs.empty()) tabs[0]->setSelected (true);
}

void AMTabBar::setStyle (AMTab::Style s)
{
    style = s;
    for (auto& t : tabs) t->setStyle (s);
}

void AMTabBar::setSelected (int index, juce::NotificationType notify)
{
    index = juce::jlimit (0, (int) tabs.size() - 1, index);
    if (index == selected) return;
    selected = index;
    for (int i = 0; i < (int) tabs.size(); ++i) tabs[(size_t) i]->setSelected (i == selected);
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

void AMTabBar::resized()
{
    if (tabs.empty()) return;
    auto area = getLocalBounds();
    const int w = area.getWidth() / (int) tabs.size();
    for (auto& t : tabs) t->setBounds (area.removeFromLeft (w));
}

void AMTabBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (style == AMTab::Style::Strip)
    {
        g.setColour (Theme::borderSoft);
        g.drawLine (b.getX(), b.getBottom() - 1.0f, b.getRight(), b.getBottom() - 1.0f, 1.0f);
    }
}

} // namespace am::ui
