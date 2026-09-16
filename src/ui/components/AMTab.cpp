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
    const auto pair = Theme::accentPair (accent);
    const auto col = Theme::textSecondary.interpolatedWith (Theme::textPrimary, juce::jmax (on, hv * 0.7f));
    const float corner = juce::jlimit (4.0f, 11.0f, b.getHeight() * 0.17f);

    // The active page is a raised white pill with an accent underline (SPEC section 6).
    const float margin = juce::jmax (2.0f, juce::jmin (b.getWidth() * 0.05f, b.getHeight() * 0.09f));
    const auto key = b.reduced (juce::jmax (margin, b.getWidth() * 0.035f), margin);
    if (on > 0.02f)
    {
        draw::selectedCell (g, key, corner, accent, on, true, margin);
    }
    else if (hv > 0.02f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.5f * hv));
        g.fillRoundedRectangle (key, corner);
    }

    if (style == Style::Nav && icon.has_value() && b.getHeight() > 30.0f)
    {
        const float labelH = juce::jlimit (8.5f, 11.0f, b.getHeight() * 0.16f);
        auto iconArea = b.withTrimmedBottom (labelH * 2.4f).reduced (0.0f, b.getHeight() * 0.22f);
        const float d = juce::jmin (iconArea.getWidth(), iconArea.getHeight(), 18.0f);
        iconArea = iconArea.withSizeKeepingCentre (d, d);
        if (on > 0.02f) draw::glowEllipse (g, iconArea, pair.second, d * 0.5f, 0.30f * on);
        Icons::draw (g, *icon, iconArea, on > 0.5f ? pair.first.interpolatedWith (Theme::textPrimary, 0.3f) : col, 0.85f);
        draw::trackedText (g, upper, b.withTop (iconArea.getBottom() + 3.0f).withTrimmedBottom (b.getHeight() * 0.11f),
                           juce::Justification::centredTop,
                           on > 0.5f ? Theme::labelFontStrong (labelH) : Theme::labelFont (labelH), col);
    }
    else
    {
        const float h = juce::jlimit (8.5f, 12.0f, b.getHeight() * 0.36f);
        draw::trackedText (g, upper, on > 0.5f ? b.withTrimmedBottom (b.getHeight() * 0.11f) : b,
                           juce::Justification::centred, on > 0.5f ? Theme::labelFontStrong (h) : Theme::labelFont (h), col);
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
        draw::hairline (g, b.getX(), b.getBottom() - 1.5f, b.getRight(), b.getBottom() - 1.5f, 0.07f);
}

} // namespace am::ui
