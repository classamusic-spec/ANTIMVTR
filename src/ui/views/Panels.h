#pragma once

#include "Controls.h"
#include "ui/components/AMSourceSelector.h"
#include "ui/components/AMWaveView.h"
#include "ui/components/AMSpaceArt.h"
#include "ui/visualizers/SpectrumAnalyzer.h"

namespace am::ui
{

//==============================================================================
/** SOURCE — choose your energy. */
class SourcePanel : public AMPanel, private juce::Timer
{
public:
    explicit SourcePanel (AntiMatrProcessor& p);
    ~SourcePanel() override { stopTimer(); }
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
class ShapePanel : public AMPanel, private juce::Timer
{
public:
    explicit ShapePanel (AntiMatrProcessor& p);
    ~ShapePanel() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    void setAdvanced (bool advanced);
    AntiMatrProcessor& processor;
    AMSegment mode { { "Simple", "Advanced" }, Theme::cyan };
    std::vector<std::unique_ptr<BoundKnob>> simpleKnobs;
    std::vector<std::unique_ptr<BoundControl>> advancedControls;
    bool advanced = false;
};

//==============================================================================
/** EVOLVE — movement & change. */
class EvolvePanel : public AMPanel, private juce::Timer
{
public:
    explicit EvolvePanel (AntiMatrProcessor& p);
    ~EvolvePanel() override { stopTimer(); }
    void resized() override;

    /** Operator tile: line icon, label and a thin amount bar. Shared with the Evolve page. */
    class OperatorCell : public juce::Component,
                         public juce::SettableTooltipClient
    {
    public:
        OperatorCell (const juce::String& label, Icon icon);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseEnter (const juce::MouseEvent&) override { anim.animate (hover, 1.0f); }
        void mouseExit (const juce::MouseEvent&) override { anim.animate (hover, 0.0f); }
        void setSelected (bool on);
        void setAmount (float a) { if (std::abs (a - amount) > 0.01f) { amount = a; repaint(); } }
        std::function<void()> onClick;
    private:
        juce::String name;
        Icon glyph;
        bool selected = false;
        float amount = 0.0f;
        Eased hover, lit;
        Animator anim { *this, { &hover, &lit } };
    };

    static Param operatorParam (int index);

private:
    void timerCallback() override;
    void selectOperator (int index, bool fromParameter);

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
    ~FracturePanel() override { stopTimer(); }
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
    ~SpacePanel() override { stopTimer(); }
    void resized() override;

    /** "‹ NEBULA ›" picker beside a procedural environment thumbnail. Shared with the Space page. */
    class SpacePicker : public juce::Component,
                        public juce::SettableTooltipClient
    {
    public:
        SpacePicker();
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseMove (const juce::MouseEvent& e) override;
        void mouseExit (const juce::MouseEvent&) override { hoverZone = 0; repaint(); }
        std::function<void (int)> onArrow;
        std::function<void (int)> onSelect;   ///< direct choice from the popup
        /** Art on the right (default) or art filling the whole component with the name overlaid. */
        void setArtOnly (bool b) { artOnly = b; repaint(); }
        int type = 0;
        float phase = 0.0f;
        float activity = 0.0f;
    private:
        int zoneAt (juce::Point<int> p) const;
        juce::Rectangle<float> nameBounds() const;
        int hoverZone = 0;
        bool artOnly = false;
    };

private:
    void timerCallback() override;
    AntiMatrProcessor& processor;
    SpacePicker picker;
    std::unique_ptr<juce::ParameterAttachment> typeAttachment;
    std::vector<std::unique_ptr<BoundKnob>> knobs;
};

} // namespace am::ui
