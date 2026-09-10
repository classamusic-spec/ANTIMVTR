#pragma once

#include "AntiMatrProcessor.h"
#include "ui/AntiMatrLookAndFeel.h"
#include "ui/views/MainView.h"
#include "ui/views/TopBar.h"
#include "ui/components/AMTooltip.h"

namespace am
{
namespace dev { class DSPLabView; }

/**
    The plugin editor: resizable (fixed 1.6 aspect ratio), page container with
    the Main page always one click away, and the hidden DSP LAB in dev builds.
*/
class AntiMatrEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit AntiMatrEditor (AntiMatrProcessor& p);
    ~AntiMatrEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

    void showPage (int index);
    int  currentPage() const noexcept { return page; }
    int  numPages() const noexcept { return 1 + (int) pages.size() + (labView != nullptr ? 1 : 0); }

    /** The DSP LAB view (dev builds only, else nullptr). */
    juce::Component* labViewComponent() noexcept { return labView.get(); }

    /** The component behind a navigation index (0 = MAIN), for tools that drive the editor. */
    juce::Component* pageComponent (int index) noexcept
    {
        if (index <= 0) return &mainView;
        const int deep = index - 1;
        return deep < (int) pages.size() ? pages[(size_t) deep].get() : labView.get();
    }

    static constexpr int kMinWidth  = 1100;
    static constexpr int kMinHeight = 690;

private:
    void timerCallback() override;

    AntiMatrProcessor& processor;
    ui::AntiMatrLookAndFeel lookAndFeel;
    ui::TopBar topBar;
    ui::NavBar navBar;
    ui::MainView mainView;
    std::vector<std::unique_ptr<juce::Component>> pages;   // index 1.. = deep pages
    std::unique_ptr<juce::Component> labView;
    ui::AMTooltipWindow tooltips { this, 600 };
    int page = 0;
    int frameCounter = 0;
};

} // namespace am
