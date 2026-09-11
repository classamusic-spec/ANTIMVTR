#include "AntiMatrLookAndFeel.h"
#include "components/AMDrawing.h"

namespace am::ui
{

AntiMatrLookAndFeel::AntiMatrLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour (juce::PopupMenu::backgroundColourId, Theme::panel);
    setColour (juce::PopupMenu::textColourId, Theme::textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::blue.withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::textPrimary);
    setColour (juce::PopupMenu::headerTextColourId, Theme::textSecondary);
    setColour (juce::AlertWindow::backgroundColourId, Theme::panel);
    setColour (juce::AlertWindow::textColourId, Theme::textPrimary);
    setColour (juce::AlertWindow::outlineColourId, Theme::border);
    setColour (juce::TextEditor::backgroundColourId, Theme::panelInset);
    setColour (juce::TextEditor::textColourId, Theme::textPrimary);
    setColour (juce::TextEditor::outlineColourId, Theme::border);
    setColour (juce::TextEditor::focusedOutlineColourId, Theme::blue.withAlpha (0.6f));
    setColour (juce::TextEditor::highlightColourId, Theme::blue.withAlpha (0.35f));
    setColour (juce::Label::textColourId, Theme::textPrimary);
    setColour (juce::TextButton::buttonColourId, Theme::panelTop);
    setColour (juce::TextButton::buttonOnColourId, Theme::blue.withAlpha (0.2f));
    setColour (juce::TextButton::textColourOffId, Theme::textSecondary);
    setColour (juce::TextButton::textColourOnId, Theme::textPrimary);
    setColour (juce::ComboBox::backgroundColourId, Theme::panelInset);
    setColour (juce::ComboBox::textColourId, Theme::textPrimary);
    setColour (juce::ComboBox::outlineColourId, Theme::border);
    setColour (juce::ComboBox::arrowColourId, Theme::textSecondary);
    setColour (juce::ToggleButton::textColourId, Theme::textPrimary);
    setColour (juce::ToggleButton::tickColourId, Theme::cyan);
    setColour (juce::ScrollBar::thumbColourId, Theme::textDim);
    setColour (juce::ListBox::textColourId, Theme::textPrimary);
    setColour (juce::TooltipWindow::backgroundColourId, Theme::panelTop);
    setColour (juce::TooltipWindow::textColourId, Theme::textPrimary);
    setColour (juce::TooltipWindow::outlineColourId, Theme::border);
    setColour (juce::ListBox::backgroundColourId, Theme::panelInset);
    setColour (juce::TableHeaderComponent::backgroundColourId, Theme::panelTop);
    setColour (juce::TableHeaderComponent::textColourId, Theme::textSecondary);
    setColour (juce::TableHeaderComponent::outlineColourId, Theme::border);
    setColour (juce::Slider::thumbColourId, Theme::cyan);
    setColour (juce::Slider::trackColourId, Theme::knobTrack);
    setColour (juce::Slider::backgroundColourId, Theme::panelInset);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff6f7f9));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1a1a22));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::cyan.withAlpha (0.6f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::blue.withAlpha (0.3f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffa7adb8));
}

void AntiMatrLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    // A menu is a small slab of the same frosted glass, lifted off the chassis.
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    draw::SlabStyle style;
    style.top    = juce::Colours::white;
    style.bottom = Theme::panelTop;
    style.shadow = 0.0f;   // the menu window has no room outside itself for one
    draw::raisedSlab (g, b, 6.0f, style);
    // With no shadow to hold it off the ground, a menu needs one fine edge of its own.
    g.setColour (Theme::textPrimary.withAlpha (0.20f));
    g.drawRoundedRectangle (b, 6.0f, 1.0f);
}

juce::Font AntiMatrLookAndFeel::getPopupMenuFont() { return Theme::font (13.0f); }

void AntiMatrLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                                             bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                                             const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused (icon, shortcutKeyText);
    if (isSeparator)
    {
        g.setColour (Theme::border);
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.reduced (2, 1);
    if (isHighlighted && isActive)
    {
        g.setColour (Theme::blue.withAlpha (0.18f));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
    }
    auto colour = textColour != nullptr ? *textColour : (isActive ? Theme::textPrimary : Theme::textDim);
    g.setColour (colour);
    g.setFont (getPopupMenuFont());
    auto textArea = r.reduced (10, 0);
    if (isTicked)
    {
        g.setColour (Theme::cyan);
        g.fillEllipse (textArea.removeFromLeft (10).toFloat().withSizeKeepingCentre (5.0f, 5.0f));
        textArea.removeFromLeft (4);
        g.setColour (colour);
    }
    else textArea.removeFromLeft (14);
    if (hasSubMenu)
    {
        auto arrow = textArea.removeFromRight (12).toFloat();
        juce::Path p; p.startNewSubPath (arrow.getX() + 3, arrow.getCentreY() - 4); p.lineTo (arrow.getX() + 8, arrow.getCentreY()); p.lineTo (arrow.getX() + 3, arrow.getCentreY() + 4);
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }
    g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
}

void AntiMatrLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical,
                                         int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused (isMouseDown);
    juce::Rectangle<int> thumb = isScrollbarVertical ? juce::Rectangle<int> (x + width / 4, thumbStartPosition, width / 2, thumbSize)
                                                     : juce::Rectangle<int> (thumbStartPosition, y + height / 4, thumbSize, height / 2);
    g.setColour (isMouseOver ? Theme::textSecondary : Theme::textDim);
    g.fillRoundedRectangle (thumb.toFloat(), 3.0f);
}

namespace
{
    constexpr float kTooltipMaxWidth = 320.0f;
    constexpr int   kTooltipPadX = 14, kTooltipPadY = 8;

    /** First line in the label face, the rest as secondary body copy. */
    juce::TextLayout tooltipLayout (const juce::String& text, float maxWidth)
    {
        juce::AttributedString s;
        const auto lines = juce::StringArray::fromLines (text);
        for (int i = 0; i < lines.size(); ++i)
        {
            if (i == 0) s.append (lines[i].toUpperCase() + (lines.size() > 1 ? "\n" : ""), Theme::labelFont (10.5f), Theme::textPrimary);
            else        s.append (lines[i] + (i + 1 < lines.size() ? "\n" : ""), Theme::bodyFont (11.5f), Theme::textSecondary);
        }
        s.setLineSpacing (2.0f);
        juce::TextLayout layout;
        layout.createLayout (s, maxWidth);
        return layout;
    }
}

juce::Rectangle<int> AntiMatrLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    const auto layout = tooltipLayout (tipText, kTooltipMaxWidth);
    const int w = juce::roundToInt (layout.getWidth()) + kTooltipPadX * 2 + 2;
    const int h = juce::roundToInt (layout.getHeight()) + kTooltipPadY * 2;
    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
               .constrainedWithin (parentArea);
}

void AntiMatrLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    draw::SlabStyle style;
    style.top    = juce::Colours::white;
    style.bottom = Theme::panelTop;
    style.shadow = 0.0f;
    style.brush  = 0.5f;
    draw::raisedSlab (g, b, 6.0f, style);
    g.setColour (Theme::textPrimary.withAlpha (0.20f));
    g.drawRoundedRectangle (b, 6.0f, 1.0f);
    // accent bar
    juce::Path bar; bar.startNewSubPath (b.getX() + 5.0f, b.getY() + 7.0f); bar.lineTo (b.getX() + 5.0f, b.getBottom() - 7.0f);
    draw::glowPath (g, bar, Theme::blue, 1.5f, 6.0f, 0.6f);
    tooltipLayout (text, kTooltipMaxWidth).draw (g, b.reduced ((float) kTooltipPadX, (float) kTooltipPadY).withTrimmedLeft (2.0f));
}

void AntiMatrLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted, bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    if (down)     draw::capsuleTrack (g, b, 6.0f, 0.9f);
    else if (on)  draw::selectedCell (g, b, 6.0f, Theme::blue, 1.0f);
    else          draw::keySlab (g, b, 6.0f, highlighted ? 0.6f : 0.0f, 0.8f);
}

void AntiMatrLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (down);
    auto b = button.getLocalBounds().toFloat();
    auto box = b.removeFromLeft (b.getHeight()).reduced (4.0f);
    draw::capsuleTrack (g, box, 3.0f, 1.0f);
    if (highlighted)
    {
        g.setColour (Theme::blue.withAlpha (0.35f));
        g.drawRoundedRectangle (box, 3.0f, 1.0f);
    }
    if (button.getToggleState())
    {
        draw::glowRoundedRect (g, box.reduced (3.0f), 2.0f, Theme::blue, 5.0f, 0.6f);
        g.setColour (Theme::blue);
        g.fillRoundedRectangle (box.reduced (3.0f), 2.0f);
    }
    g.setColour (Theme::textPrimary);
    g.setFont (Theme::font (12.0f));
    g.drawFittedText (button.getButtonText(), b.reduced (4, 0).toNearestInt(), juce::Justification::centredLeft, 1);
}

void AntiMatrLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    draw::capsuleTrack (g, b, 5.0f, 1.0f);
    if (box.hasKeyboardFocus (true))
    {
        g.setColour (Theme::blue.withAlpha (0.5f));
        g.drawRoundedRectangle (b, 5.0f, 1.0f);
    }
    auto arrow = b.removeFromRight ((float) height).reduced ((float) height * 0.32f);
    juce::Path p; p.startNewSubPath (arrow.getX(), arrow.getY() + arrow.getHeight() * 0.35f); p.lineTo (arrow.getCentreX(), arrow.getBottom() - arrow.getHeight() * 0.35f); p.lineTo (arrow.getRight(), arrow.getY() + arrow.getHeight() * 0.35f);
    g.setColour (Theme::textSecondary);
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

} // namespace am::ui
