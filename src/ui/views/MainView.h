#pragma once

#include "Panels.h"
#include "ui/visualizers/AntiMatterVisualizer.h"

namespace am::ui
{

/** The Main page: Source / Object / Shape on top, Evolve / Fracture / Space below. */
class MainView : public juce::Component
{
public:
    explicit MainView (AntiMatrProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override {}

    AntiMatterVisualizer& visualizer() noexcept { return object; }

private:
    AntiMatrProcessor& processor;
    SourcePanel   source;
    ShapePanel    shape;
    EvolvePanel   evolve;
    FracturePanel fracture;
    SpacePanel    space;
    AntiMatterVisualizer object;
};

/**
    Deep page container. The editor creates one per section with the
    section's parameter groups; the container instantiates the designed
    page for that section (SourcePage, ShapePage, EvolvePage, FracturePage,
    SpacePage, ModPage) and falls back to a registry-driven knob grid for
    any group set without a dedicated design.
*/
class GroupPage : public juce::Component
{
public:
    GroupPage (AntiMatrProcessor& p, const juce::String& title, const juce::String& subtitle,
               std::vector<ParamGroup> groups, juce::Colour accent);
    void resized() override;

    /** The designed page inside this group, when there is one (used to reach its sub-tabs). */
    juce::Component* designedPage() const noexcept { return page.get(); }

private:
    struct Section
    {
        std::unique_ptr<AMPanel> panel;
        std::vector<std::unique_ptr<BoundKnob>> knobs;
    };
    std::unique_ptr<juce::Component> page;   // designed page (if any)
    std::vector<Section> sections;           // generic fallback
    juce::Viewport viewport;
    juce::Component content;
    juce::Colour accent;
};

} // namespace am::ui
