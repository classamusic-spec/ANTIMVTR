#pragma once

#include "AntiMatrTheme.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace am::ui
{

/**
    Look-and-feel for the generic JUCE widgets that appear around the custom
    components (popup menus, alert windows, scrollbars, tooltips). The custom
    AM* components draw themselves.
*/
class AntiMatrLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AntiMatrLookAndFeel();

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    void drawScrollbar (juce::Graphics& g, juce::ScrollBar& bar, int x, int y, int width, int height, bool isScrollbarVertical,
                        int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;
    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return Theme::labelFont (11.0f); }
    juce::Font getLabelFont (juce::Label&) override { return Theme::font (12.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override { return Theme::labelFont (juce::jlimit (9.0f, 12.0f, (float) buttonHeight * 0.4f)); }
};

} // namespace am::ui
