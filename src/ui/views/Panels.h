#pragma once

#include "ui/components/AMPanel.h"
#include "ui/components/AMKnob.h"
#include "ui/components/AMButton.h"
#include "ui/components/AMSourceSelector.h"
#include "ui/components/AMWaveView.h"
#include "ui/visualizers/SpectrumAnalyzer.h"
#include "plugin/AntiMatrProcessor.h"

namespace am::ui
{

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

/** Helper: creates a knob bound to a parameter. */
struct BoundKnob
{
    BoundKnob (juce::AudioProcessorValueTreeState& apvts, Param p, juce::Colour accent, const juce::String& labelOverride = {});
    AMKnob knob;
    std::unique_ptr<SliderAttachment> attachment;
};

/** Lays out knobs evenly in a row (or grid with `rows`). */
void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> knobs, int rows = 1);

//==============================================================================
/** SOURCE — choose your energy. */
class SourcePanel : public AMPanel, private juce::Timer
{
public:
    explicit SourcePanel (AntiMatrProcessor& p);
    void resized() override;

private:
    void timerCallback() override;
    void rebuildKnobs (int sourceIndex);

    AntiMatrProcessor& processor;
    AMSourceSelector selector;
    std::unique_ptr<juce::ParameterAttachment> selectorAttachment;
    AMWaveView wave;
    std::vector<std::unique_ptr<BoundKnob>> knobs;
    int currentSource = -1;
    std::array<float, 1024> tapL {}, tapR {};
};

//==============================================================================
/** SHAPE — turn matter into sound. */
class ShapePanel : public AMPanel
{
public:
    explicit ShapePanel (AntiMatrProcessor& p);
    void resized() override;

private:
    void setAdvanced (bool advanced);
    AntiMatrProcessor& processor;
    AMSegment mode { { "Simple", "Advanced" }, Theme::cyan };
    std::vector<std::unique_ptr<BoundKnob>> simpleKnobs, advancedKnobs;
    bool advanced = false;
};

//==============================================================================
/** EVOLVE — movement & change. */
class EvolvePanel : public AMPanel, private juce::Timer
{
public:
    explicit EvolvePanel (AntiMatrProcessor& p);
    void resized() override;

private:
    class OperatorCell : public juce::Component
    {
    public:
        OperatorCell (const juce::String& label, Icon icon) : name (label), glyph (icon) { setWantsKeyboardFocus (false); }
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
        std::function<void()> onClick;
        bool selected = false;
        float amount = 0.0f;
        bool hover = false;
        juce::String name;
        Icon glyph;
    };

    void timerCallback() override;
    void selectOperator (int index, bool fromParameter);
    static Param operatorParam (int index);

    AntiMatrProcessor& processor;
    std::array<std::unique_ptr<OperatorCell>, 4> cells;
    std::unique_ptr<juce::ParameterAttachment> selectedAttachment;
    AMSlider amount { "Amount", Theme::violet };
    AMSlider speed { "Speed", Theme::violet };
    std::unique_ptr<SliderAttachment> amountAttachment, speedAttachment;
    int selectedOperator = 0;
};

//==============================================================================
/** FRACTURE — break into new realities. */
class FracturePanel : public AMPanel, private juce::Timer
{
public:
    explicit FracturePanel (AntiMatrProcessor& p);
    void resized() override;

private:
    void timerCallback() override;
    AntiMatrProcessor& processor;
    AMSegment onOff { { "Off", "On" }, Theme::magenta };
    std::unique_ptr<juce::ParameterAttachment> onAttachment;
    AMSpectrumView spectrum;
    SpectrumAnalyzer analyzer;
    std::array<float, SpectrumAnalyzer::kSize> tapL {}, tapR {};
    std::array<float, AMSpectrumView::kBands> bands {};
    std::vector<std::unique_ptr<BoundKnob>> knobs;
};

//==============================================================================
/** SPACE — place it anywhere. */
class SpacePanel : public AMPanel, private juce::Timer
{
public:
    explicit SpacePanel (AntiMatrProcessor& p);
    void resized() override;

private:
    class SpacePicker : public juce::Component
    {
    public:
        SpacePicker();
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        std::function<void (int)> onArrow;
        int type = 0;
        float phase = 0.0f;
        float activity = 0.0f;
    };

    void timerCallback() override;
    AntiMatrProcessor& processor;
    SpacePicker picker;
    std::unique_ptr<juce::ParameterAttachment> typeAttachment;
    std::vector<std::unique_ptr<BoundKnob>> knobs;
};

} // namespace am::ui
