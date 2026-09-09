#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMIcons.h"

namespace am::ui
{

/**
    Tab: icon + label (navigation style) or label-only strip. The selected
    tab carries a glowing accent underline; hover and selection are eased.
*/
class AMTab : public juce::Component
{
public:
    enum class Style { Nav, Strip };

    AMTab (const juce::String& name, std::optional<Icon> icon = std::nullopt, juce::Colour accent = Theme::blue);

    void setSelected (bool on);
    bool isSelected() const noexcept { return selected; }
    void setStyle (Style s) { style = s; repaint(); }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    const juce::String& getName() const noexcept { return name; }
    std::function<void()> onClick;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void mouseEnter (const juce::MouseEvent&) override { anim.animate (hover, 1.0f); }
    void mouseExit (const juce::MouseEvent&) override { anim.animate (hover, 0.0f); }

private:
    juce::String name, upper;
    std::optional<Icon> icon;
    juce::Colour accent;
    Style style = Style::Nav;
    bool selected = false;
    Eased hover, lit;
    Animator anim { *this, { &hover, &lit } };
};

/** Horizontal row of AMTabs with a single selection. */
class AMTabBar : public juce::Component
{
public:
    AMTabBar (juce::StringArray names, juce::Colour accent = Theme::blue, std::vector<Icon> icons = {});

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int getSelected() const noexcept { return selected; }
    void setStyle (AMTab::Style s);
    std::function<void (int)> onChange;
    int numTabs() const noexcept { return (int) tabs.size(); }
    AMTab& tab (int i) { return *tabs[(size_t) i]; }

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    std::vector<std::unique_ptr<AMTab>> tabs;
    int selected = 0;
    AMTab::Style style = AMTab::Style::Strip;
};

} // namespace am::ui
