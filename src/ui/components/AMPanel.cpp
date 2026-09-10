#include "AMPanel.h"
#include "ui/UILayout.h"

namespace am::ui
{

AMPanel::AMPanel (const juce::String& t, const juce::String& s, juce::Colour a)
    : title (t.toUpperCase()), subtitle (s.toUpperCase()), accent (a)
{
    setInterceptsMouseClicks (false, true);
}

juce::Rectangle<float> AMPanel::slabBounds() const
{
    const auto b = getLocalBounds().toFloat();
    return b.reduced (layout::panelShadowMargin (b.getWidth(), b.getHeight()));
}

int AMPanel::padding() const
{
    return juce::roundToInt (layout::panelPadding (slabBounds().getWidth()));
}

juce::Rectangle<int> AMPanel::headerBounds() const
{
    // The floor keeps room for the title *and* its subtitle, so a short panel does not
    // silently drop the subtitle its neighbours are showing.
    const int h = compact ? juce::jlimit (26, 40, juce::roundToInt ((float) getHeight() * 0.12f))
                          : juce::jlimit (38, 64, juce::roundToInt ((float) getHeight() * 0.155f));
    const int pad = padding();
    return slabBounds().toNearestInt().withHeight (h).reduced (pad, 0).withTrimmedTop (pad / 2);
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
    return slabBounds().toNearestInt().withTop (h.getBottom() + pad / 2).reduced (pad, 0).withTrimmedBottom (pad);
}

void AMPanel::paint (juce::Graphics& g)
{
    const auto b = slabBounds();
    // SPEC section 1: a corner radius of about 1.2 % of the editor width, taken from
    // the panel's own bounds so a short panel is not over-rounded.
    const float corner = juce::jlimit (5.0f, Theme::kPanelRadius, juce::jmin (b.getWidth() * 0.045f, b.getHeight() * 0.16f));

    if (activity > 0.02f)
        draw::glowRoundedRect (g, b, corner, accent, 16.0f, activity * 0.35f);

    draw::SlabStyle style;
    // The shadow reaches exactly as far as the margin the slab was inset by, so the
    // whole of it lands inside the component.
    style.shadowRadius = layout::panelShadowMargin ((float) getWidth(), (float) getHeight()) * 1.1f;
    draw::raisedSlab (g, b, corner, style);

    // Four screws bolt the slab to the chassis, one inset from each corner.
    const auto hardware = layout::panelHardware (b.getWidth(), b.getHeight());
    draw::rivets (g, b, hardware.inset, hardware.radius);

    const auto h = headerBounds().toFloat();
    const float titleH = compact ? juce::jlimit (10.0f, 14.0f, h.getHeight() * 0.5f)
                                 : juce::jlimit (11.0f, 17.0f, h.getHeight() * 0.36f);
    const float subH   = juce::jlimit (7.5f, 10.0f, h.getHeight() * 0.2f);

    auto titleArea = h.withHeight (titleH * 1.45f);
    if (compact) titleArea = h;
    const float titleMaxW = (headerRightBoundsUsed ? h.getWidth() * 0.5f : h.getWidth()) - 4.0f;
    draw::trackedText (g, title, titleArea, juce::Justification::centredLeft,
                       draw::fitFont (Theme::titleFont (titleH), title, titleMaxW, 8.0f), Theme::textPrimary);

    if (! compact && subtitle.isNotEmpty() && h.getHeight() > titleH * 1.35f + subH * 1.2f)
    {
        auto subArea = h.withTop (titleArea.getBottom() - 1.0f).withHeight (subH * 1.5f);
        draw::trackedText (g, subtitle, subArea, juce::Justification::centredLeft, Theme::captionFont (subH), Theme::textSecondary);
    }

    if (showAccentLine)
    {
        const float lineY = h.getBottom() + (compact ? 1.0f : 3.0f);
        const float lineW = juce::jmin (h.getWidth() * 0.26f, compact ? 40.0f : 84.0f);
        const auto pair = Theme::accentPair (accent);

        // The underline is cut into the slab, then lit in the section colours.
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (h.getX(), lineY + 1.2f, h.getRight(), lineY + 1.2f, 1.0f);

        juce::Path line;
        line.startNewSubPath (h.getX(), lineY);
        line.lineTo (h.getX() + lineW, lineY);
        {
            juce::ColourGradient grad (pair.first, h.getX(), lineY, pair.second, h.getX() + lineW, lineY, false);
            g.setGradientFill (grad);
            g.strokePath (line, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        draw::glowPath (g, line, pair.second, 1.0f, 7.0f, (0.35f + 0.55f * activity) * 0.9f);

        g.setColour (Theme::borderSoft);
        g.drawLine (h.getX() + lineW, lineY, h.getRight(), lineY, 1.0f);
    }
}

} // namespace am::ui
