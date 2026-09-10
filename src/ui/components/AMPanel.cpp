#include "AMPanel.h"

namespace am::ui
{

AMPanel::AMPanel (const juce::String& t, const juce::String& s, juce::Colour a)
    : title (t.toUpperCase()), subtitle (s.toUpperCase()), accent (a)
{
    setInterceptsMouseClicks (false, true);
}

int AMPanel::padding() const
{
    return juce::jlimit (8, 22, juce::roundToInt ((float) getWidth() * 0.042f));
}

juce::Rectangle<int> AMPanel::headerBounds() const
{
    const int h = compact ? juce::jlimit (26, 40, juce::roundToInt ((float) getHeight() * 0.12f))
                          : juce::jlimit (34, 64, juce::roundToInt ((float) getHeight() * 0.155f));
    const int pad = padding();
    return getLocalBounds().withHeight (h).reduced (pad, 0).withTrimmedTop (pad / 2);
}

juce::Rectangle<int> AMPanel::headerRightBounds() const
{
    headerRightBoundsUsed = true;
    auto h = headerBounds();
    return h.removeFromRight (h.getWidth() / 2);
}

juce::Rectangle<int> AMPanel::contentBounds() const
{
    const auto h = headerBounds();
    const int pad = padding();
    return getLocalBounds().withTrimmedTop (h.getBottom() + pad / 2).reduced (pad, 0).withTrimmedBottom (pad);
}

void AMPanel::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jlimit (6.0f, Theme::kPanelRadius, b.getWidth() * 0.03f);

    if (activity > 0.02f)
        draw::glowRoundedRect (g, b, corner, accent, 16.0f, activity * 0.35f);

    draw::panelSurface (g, b, corner);

    const auto h = headerBounds().toFloat();
    const float titleH = compact ? juce::jlimit (10.0f, 14.0f, h.getHeight() * 0.5f)
                                 : juce::jlimit (11.0f, 18.0f, h.getHeight() * 0.38f);
    const float subH   = juce::jlimit (7.5f, 10.0f, h.getHeight() * 0.2f);

    auto titleArea = h.withHeight (titleH * 1.35f);
    if (compact) titleArea = h;
    const float titleMaxW = (headerRightBoundsUsed ? h.getWidth() * 0.5f : h.getWidth()) - 4.0f;
    draw::trackedText (g, title, titleArea, juce::Justification::centredLeft, draw::fitFont (Theme::titleFont (titleH), title, titleMaxW, 8.0f), Theme::textPrimary);

    if (! compact && subtitle.isNotEmpty() && h.getHeight() > titleH * 1.35f + subH * 1.2f)
    {
        auto subArea = h.withTop (titleArea.getBottom() - 1.0f).withHeight (subH * 1.5f);
        draw::trackedText (g, subtitle, subArea, juce::Justification::centredLeft, Theme::captionFont (subH), Theme::textSecondary);
    }

    if (showAccentLine)
    {
        const float lineY = h.getBottom() + (compact ? 1.0f : 3.0f);
        const float lineW = juce::jmin (h.getWidth() * 0.26f, compact ? 40.0f : 84.0f);
        juce::Path line;
        line.startNewSubPath (h.getX(), lineY);
        line.lineTo (h.getX() + lineW, lineY);
        draw::glowPath (g, line, accent, 1.2f, 7.0f, 0.45f + 0.55f * activity);

        g.setColour (Theme::borderSoft);
        g.drawLine (h.getX() + lineW, lineY, h.getRight(), lineY, 1.0f);
    }
}

} // namespace am::ui
