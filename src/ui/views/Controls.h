#pragma once

#include "ui/components/AMPanel.h"
#include "ui/components/AMKnob.h"
#include "ui/components/AMButton.h"
#include "ui/components/AMControls.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::ui
{

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

/** Helper: creates a knob bound to a parameter. */
struct BoundKnob
{
    BoundKnob (juce::AudioProcessorValueTreeState& apvts, Param p, juce::Colour accent, const juce::String& labelOverride = {});
    /** Draws the live modulation ring; false when nothing is routed to this parameter. */
    bool refreshModRing (const ModulationSnapshot& s) { return knob.refreshModRing (s); }
    Param param;
    AMKnob knob;
    std::unique_ptr<SliderAttachment> attachment;
};

/**
    A control of the right kind for any parameter: knob for Float / Int,
    toggle pill for Bool, stepper for Choice. Keeps its attachment alive.
*/
class BoundControl
{
public:
    BoundControl (juce::AudioProcessorValueTreeState& apvts, Param p, juce::Colour accent, const juce::String& labelOverride = {});

    juce::Component& component() noexcept { return *comp; }
    AMKnob* knob() noexcept { return dynamic_cast<AMKnob*> (comp.get()); }
    AMToggle* toggle() noexcept { return dynamic_cast<AMToggle*> (comp.get()); }
    AMChoice* choice() noexcept { return dynamic_cast<AMChoice*> (comp.get()); }
    Param param() const noexcept { return parameter; }
    bool isKnob() const noexcept { return kind == ParamKind::Float || kind == ParamKind::Int; }

    /** Draws the live modulation ring; false when nothing is routed to this parameter. */
    bool refreshModRing (const ModulationSnapshot& s) { auto* k = knob(); return k != nullptr && k->refreshModRing (s); }

private:
    Param parameter;
    ParamKind kind;
    std::unique_ptr<juce::Component> comp;
    std::unique_ptr<SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::ParameterAttachment> paramAttachment;
};

/** Lays out components evenly in a row (or grid with `rows`). */
void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> comps, int rows = 1);
void layoutGrid (juce::Rectangle<int> area, const std::vector<juce::Component*>& comps, int columns, int gapX = 0, int gapY = 0);

/** Returns the choices of a Choice parameter as a StringArray. */
juce::StringArray paramChoices (Param p);

/** Human-readable tooltip for a parameter. */
juce::String paramTooltip (Param p);

/** The most recent modulation snapshot published by the engine (message thread). */
const ModulationSnapshot& latestModulation (AntiMatrProcessor& p);

} // namespace am::ui
