#pragma once

#include "ui/components/AMLogo.h"
#include "ui/components/AMButton.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::ui
{

/** Branding, preset selector, BROWSE / RANDOM / MUTATE and settings. */
class TopBar : public juce::Component,
               private juce::ChangeListener,
               private juce::Timer
{
public:
    explicit TopBar (AntiMatrProcessor& p);
    ~TopBar() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onBrowse;
    std::function<void()> onSettings;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    void timerCallback() override;
    void showMutateMenu();
    void showSettingsMenu();
    void showBrowseMenu();

    AntiMatrProcessor& processor;
    AMLogo logo;
    AMIconButton prev { Icon::ChevronLeft }, next { Icon::ChevronRight }, settings { Icon::Settings };
    AMButton browse { "Browse" }, random { "Random" }, mutate { "Mutate" };
    juce::Rectangle<int> presetArea;
};

/** Bottom navigation: pages, A/B, brand caption. */
class NavBar : public juce::Component
{
public:
    explicit NavBar (AntiMatrProcessor& p, bool showLab);

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setPage (int index);
    int getPage() const noexcept { return page; }
    std::function<void (int)> onPageChange;
    std::function<void (int)> onABChange;

    static juce::StringArray pageNames();

private:
    struct Tab : public juce::Component
    {
        Tab (const juce::String& n, Icon i) : name (n), icon (i) { setWantsKeyboardFocus (false); }
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
        std::function<void()> onClick;
        juce::String name; Icon icon; bool selected = false; bool hover = false;
    };

    AntiMatrProcessor& processor;
    std::vector<std::unique_ptr<Tab>> tabs;
    AMSegment ab { { "A", "B" }, Theme::cyan };
    int page = 0;
    bool lab = false;
};

} // namespace am::ui
