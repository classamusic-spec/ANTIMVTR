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
    Generic deep page: every parameter of the given groups as bound knobs in a
    grid. Functional from day one; dedicated designs replace these in later
    phases without changing the parameter contract.
*/
class GroupPage : public juce::Component
{
public:
    GroupPage (AntiMatrProcessor& p, const juce::String& title, const juce::String& subtitle,
               std::vector<ParamGroup> groups, juce::Colour accent);
    void resized() override;

private:
    struct Section
    {
        std::unique_ptr<AMPanel> panel;
        std::vector<std::unique_ptr<BoundKnob>> knobs;
    };
    std::vector<Section> sections;
    juce::Viewport viewport;
    juce::Component content;
    juce::Colour accent;
};

} // namespace am::ui
