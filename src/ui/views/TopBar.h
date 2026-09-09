#pragma once

#include "ui/components/AMLogo.h"
#include "ui/components/AMButton.h"
#include "ui/components/AMTab.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::ui
{

class PresetBrowser;

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

    /** Optional overrides; by default BROWSE opens the preset browser overlay over the whole editor. */
    std::function<void()> onBrowse;
    std::function<void()> onSettings;

    /** Opens / closes the browser overlay (added to the top-level parent so it covers every page). */
    void openBrowser();
    void closeBrowser();
    bool isBrowserOpen() const noexcept;

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
    std::unique_ptr<PresetBrowser> browser;
};

/** Bottom navigation: pages, A/B, output level, brand caption. */
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
    AntiMatrProcessor& processor;
    std::vector<std::unique_ptr<AMTab>> tabs;
    AMSegment ab { { "A", "B" }, Theme::cyan };
    AMSlider output { "Output", Theme::ivory };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    juce::Rectangle<int> swirlArea;
    int page = 0;
    bool lab = false;
};

} // namespace am::ui
