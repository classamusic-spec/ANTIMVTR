#include "AntiMatrLookAndFeel.h"
#include "components/AMDrawing.h"

namespace am::ui
{

AntiMatrLookAndFeel::AntiMatrLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour (juce::PopupMenu::backgroundColourId, Theme::panel);
    setColour (juce::PopupMenu::textColourId, Theme::textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::blue.withAlpha (0.25f));
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
    setColour (juce::TextButton::buttonOnColourId, Theme::blue.withAlpha (0.3f));
    setColour (juce::TextButton::textColourOffId, Theme::textSecondary);
    setColour (juce::TextButton::textColourOnId, Theme::textPrimary);
    setColour (juce::ComboBox::backgroundColourId, Theme::panelInset);
    setColour (juce::ComboBox::textColourId, Theme::textPrimary);
    setColour (juce::ComboBox::outlineColourId, Theme::border);
    setColour (juce::ComboBox::arrowColourId, Theme::textSecondary);
    setColour (juce::ToggleButton::textColourId, Theme::textPrimary);
    setColour (juce::ToggleButton::tickColourId, Theme::cyan);
    setColour (juce::ScrollBar::thumbColourId, Theme::textDim);
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
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd8d8e0));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1a1a22));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::cyan.withAlpha (0.6f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::blue.withAlpha (0.3f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff404050));
}

void AntiMatrLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height);
    g.fillAll (Theme::panel);
    g.setColour (Theme::border);
    g.drawRect (b, 1.0f);
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

void AntiMatrLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height);
    g.setColour (Theme::panelTop);
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (Theme::textPrimary);
    g.setFont (Theme::font (12.0f));
    g.drawFittedText (text, b.reduced (6, 2).toNearestInt(), juce::Justification::centredLeft, 3);
}

void AntiMatrLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted, bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    g.setColour (on ? Theme::blue.withAlpha (0.22f) : (highlighted || down ? Theme::glass : Theme::panelInset));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (on ? Theme::blue.withAlpha (0.5f) : Theme::border);
    g.drawRoundedRectangle (b, 6.0f, 1.0f);
}

void AntiMatrLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    juce::ignoreUnused (down);
    auto b = button.getLocalBounds().toFloat();
    auto box = b.removeFromLeft (b.getHeight()).reduced (4.0f);
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (highlighted ? Theme::textSecondary : Theme::border);
    g.drawRoundedRectangle (box, 3.0f, 1.0f);
    if (button.getToggleState())
    {
        draw::glowRoundedRect (g, box.reduced (3.0f), 2.0f, Theme::cyan, 5.0f, 0.6f);
        g.setColour (Theme::cyan);
        g.fillRoundedRectangle (box.reduced (3.0f), 2.0f);
    }
    g.setColour (Theme::textPrimary);
    g.setFont (Theme::font (12.0f));
    g.drawFittedText (button.getButtonText(), b.reduced (4, 0).toNearestInt(), juce::Justification::centredLeft, 1);
}

void AntiMatrLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (Theme::panelInset);
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? Theme::blue.withAlpha (0.5f) : Theme::border);
    g.drawRoundedRectangle (b, 5.0f, 1.0f);
    auto arrow = b.removeFromRight ((float) height).reduced ((float) height * 0.32f);
    juce::Path p; p.startNewSubPath (arrow.getX(), arrow.getY() + arrow.getHeight() * 0.35f); p.lineTo (arrow.getCentreX(), arrow.getBottom() - arrow.getHeight() * 0.35f); p.lineTo (arrow.getRight(), arrow.getY() + arrow.getHeight() * 0.35f);
    g.setColour (Theme::textSecondary);
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

} // namespace am::ui
