#include "AMOptionList.h"

namespace am::ui
{

AMOptionList::AMOptionList (juce::StringArray items, juce::Colour c)
    : names (std::move (items)), accent (c)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    lit.snap (0.0f);
}

juce::Rectangle<float> AMOptionList::descriptionBounds() const
{
    auto b = getLocalBounds().toFloat();
    if (descriptions.isEmpty()) return {};
    const float h = juce::jlimit (26.0f, 64.0f, b.getHeight() * 0.2f);
    return b.removeFromBottom (h);
}

juce::Rectangle<float> AMOptionList::listBounds() const
{
    auto b = getLocalBounds().toFloat();
    const auto desc = descriptionBounds();
    return desc.isEmpty() ? b : b.withTrimmedBottom (desc.getHeight());
}

float AMOptionList::rowHeight() const noexcept
{
    return names.isEmpty() ? 0.0f : listBounds().getHeight() / (float) names.size();
}

void AMOptionList::setSelected (int index, juce::NotificationType notify)
{
    if (names.isEmpty()) return;
    index = juce::jlimit (0, names.size() - 1, index);
    if (index == selected) return;
    selected = index;
    if (isShowing()) anim.animate (lit, (float) selected); else lit.snap ((float) selected);
    repaint();
    if (notify != juce::dontSendNotification && onChange) onChange (selected);
}

int AMOptionList::rowAt (juce::Point<int> p) const
{
    const auto list = listBounds();
    if (names.isEmpty() || ! list.contains (p.toFloat())) return -1;
    const float h = juce::jmax (1.0f, rowHeight());
    return juce::jlimit (0, names.size() - 1, (int) ((p.toFloat().y - list.getY()) / h));
}

void AMOptionList::mouseDown (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    if (row >= 0) setSelected (row);
}

void AMOptionList::mouseMove (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    if (row != hovered) { hovered = row; repaint(); }
}

void AMOptionList::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) > 0.01f) setSelected (selected + (wheel.deltaY > 0 ? -1 : 1));
}

void AMOptionList::paint (juce::Graphics& g)
{
    if (names.isEmpty()) return;
    const auto list = listBounds();
    const float rowH = rowHeight();
    if (rowH < 6.0f) return;

    const float inset = juce::jlimit (1.0f, 3.0f, rowH * 0.07f);
    const float corner = juce::jlimit (3.0f, 7.0f, rowH * 0.2f);
    const float fontHeight = juce::jlimit (9.0f, 13.0f, rowH * 0.4f);
    const float textInset = juce::jlimit (9.0f, 18.0f, list.getWidth() * 0.07f);

    // The lit row glides between entries so a change of selection reads as motion.
    {
        auto row = juce::Rectangle<float> (list.getX(), list.getY() + lit.value * rowH, list.getWidth(), rowH).reduced (0.0f, inset);
        draw::glowRoundedRect (g, row, corner, accent, rowH * 0.3f, 0.35f);
        juce::ColourGradient wash (accent.withAlpha (0.20f), row.getX(), row.getY(),
                                   accent.withAlpha (0.05f), row.getRight(), row.getY(), false);
        g.setGradientFill (wash);
        g.fillRoundedRectangle (row, corner);
        g.setColour (accent.withAlpha (0.30f));
        g.drawRoundedRectangle (row.reduced (0.5f), corner, 1.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (row.withWidth (juce::jmax (2.0f, rowH * 0.08f)), 1.5f);
    }

    for (int i = 0; i < names.size(); ++i)
    {
        auto row = juce::Rectangle<float> (list.getX(), list.getY() + (float) i * rowH, list.getWidth(), rowH).reduced (0.0f, inset);
        const bool isSelected = i == selected;
        const bool isHovered = i == hovered && ! isSelected;

        if (isHovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (row, corner);
            g.setColour (accent.withAlpha (0.18f));
            g.drawRoundedRectangle (row.reduced (0.5f), corner, 1.0f);
        }

        auto text = row.withTrimmedLeft (textInset).withTrimmedRight (textInset * 0.5f);
        draw::trackedText (g, names[i], text, juce::Justification::centredLeft,
                           draw::fitFont (isSelected ? Theme::labelFontStrong (fontHeight) : Theme::labelFont (fontHeight), names[i], text.getWidth()),
                           isSelected ? Theme::textPrimary : (isHovered ? Theme::textSecondary.brighter (0.3f) : Theme::textSecondary));
    }

    auto desc = descriptionBounds();
    if (desc.isEmpty() || selected >= descriptions.size() || descriptions[selected].isEmpty()) return;

    desc = desc.reduced (juce::jmin (textInset, desc.getWidth() * 0.1f), desc.getHeight() * 0.1f);
    g.setColour (Theme::borderSoft);
    g.drawLine (desc.getX(), desc.getY(), desc.getRight(), desc.getY(), 1.0f);

    juce::AttributedString text;
    text.append (descriptions[selected], Theme::bodyFont (juce::jlimit (10.0f, 12.5f, desc.getHeight() * 0.26f)),
                 Theme::textSecondary.brighter (0.12f));
    text.setLineSpacing (2.5f);
    juce::TextLayout layout;
    layout.createLayout (text, desc.getWidth());
    layout.draw (g, desc.withTrimmedTop (desc.getHeight() * 0.12f));
}

} // namespace am::ui
