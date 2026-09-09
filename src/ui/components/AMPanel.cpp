#include "AMPanel.h"

namespace am::ui
{

AMPanel::AMPanel (const juce::String& t, const juce::String& s, juce::Colour a)
    : title (t), subtitle (s), accent (a)
{
    setInterceptsMouseClicks (false, true);
}

juce::Rectangle<int> AMPanel::headerBounds() const
{
    const int h = juce::jlimit (34, 64, juce::roundToInt ((float) getHeight() * 0.16f));
    const int pad = juce::jlimit (8, 22, juce::roundToInt ((float) getWidth() * 0.045f));
    return getLocalBounds().withHeight (h).reduced (pad, 0).withTrimmedTop (pad / 2);
}

juce::Rectangle<int> AMPanel::headerRightBounds() const
{
    auto h = headerBounds();
    return h.removeFromRight (h.getWidth() / 2);
}

juce::Rectangle<int> AMPanel::contentBounds() const
{
    const auto h = headerBounds();
    const int pad = juce::jlimit (8, 22, juce::roundToInt ((float) getWidth() * 0.045f));
    return getLocalBounds().withTrimmedTop (h.getBottom() + pad / 2).reduced (pad, 0).withTrimmedBottom (pad);
}

void AMPanel::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jlimit (6.0f, Theme::kPanelRadius, b.getWidth() * 0.03f);

    if (activity > 0.02f)
        draw::glowRoundedRect (g, b, corner, accent, 18.0f, activity * 0.5f);

    draw::panelSurface (g, b, corner);

    const auto h = headerBounds().toFloat();
    const float titleH = juce::jlimit (11.0f, 19.0f, h.getHeight() * 0.42f);
    const float subH   = juce::jlimit (7.5f, 10.5f, h.getHeight() * 0.22f);

    auto titleArea = h.withHeight (titleH * 1.3f);
    draw::trackedText (g, title.toUpperCase(), titleArea, juce::Justification::centredLeft, Theme::titleFont (titleH), Theme::textPrimary);

    if (subtitle.isNotEmpty() && h.getHeight() > 36.0f)
    {
        auto subArea = h.withTop (titleArea.getBottom()).withHeight (subH * 1.4f);
        draw::trackedText (g, subtitle.toUpperCase(), subArea, juce::Justification::centredLeft, Theme::captionFont (subH), Theme::textSecondary);
    }

    if (showAccentLine)
    {
        const float lineY = h.getBottom() + 2.0f;
        const float lineW = juce::jmin (h.getWidth() * 0.28f, 90.0f);
        juce::Path line;
        line.startNewSubPath (h.getX(), lineY);
        line.lineTo (h.getX() + lineW, lineY);
        draw::glowPath (g, line, accent, 1.2f, 6.0f, 0.5f + 0.5f * activity);

        g.setColour (Theme::borderSoft);
        g.drawLine (h.getX() + lineW, lineY, h.getRight(), lineY, 1.0f);
    }
}

} // namespace am::ui
