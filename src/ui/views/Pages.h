#pragma once

#include "Panels.h"
#include "ModRoutingPanel.h"
#include "SourcePanels.h"
#include "ui/components/AMTab.h"
#include "ui/components/AMXYPad.h"
#include "ui/components/AMStepEditor.h"
#include "ui/components/AMOptionList.h"

namespace am::ui
{

/**
    Registry-driven panel: a titled AMPanel holding one bound control per
    parameter, laid out in a grid. Optional header toggle (module ON/OFF).
*/
class ParamPanel : public AMPanel
{
public:
    /** Labels drop `labelPrefix` (default: the title) so "Delay Feedback" reads FEEDBACK inside DELAY. */
    ParamPanel (AntiMatrProcessor& p, const juce::String& title, const juce::String& subtitle, juce::Colour accent,
                std::vector<Param> params, int columns = 0, const juce::String& labelPrefix = {});

    void resized() override;
    BoundControl* control (Param p);
    /** Places a Bool parameter's control in the header (module on/off).
        `asSwitch` uses a labelled OFF / ON segment instead of the compact pill,
        for panels big enough that an unlabelled pill would read as a mystery. */
    void setHeaderToggle (Param p, bool asSwitch = false);
    void setColumns (int c) { columns = c; resized(); }
    /** Draws the live modulation rings on every knob in the panel. */
    void refreshModRings (const ModulationSnapshot& s);
    void setHeroKnobs (bool hero);
    /** Per-parameter accent override. */
    void setAccentFor (Param p, juce::Colour c);

private:
    AntiMatrProcessor& processor;
    std::vector<std::unique_ptr<BoundControl>> controls;
    std::unique_ptr<BoundControl> headerToggle;
    std::unique_ptr<AMSegment> headerSwitch;
    std::unique_ptr<juce::ParameterAttachment> headerSwitchAttachment;
    int columns = 0;
};

//==============================================================================
/** SOURCE page: selector, waveform display and the selected source's full parameter set. */
class SourcePage : public juce::Component, private juce::Timer
{
public:
    explicit SourcePage (AntiMatrProcessor& p);
    ~SourcePage() override;
    void resized() override;

private:
    void timerCallback() override;
    void rebuild (int source);

    AntiMatrProcessor& processor;
    AMPanel sourcePanel { "Source", "Choose your energy", Theme::blue };
    AMSourceSelector selector;
    std::unique_ptr<juce::ParameterAttachment> selectorAttachment;
    std::unique_ptr<BoundControl> modeControl, levelControl;
    AMPanel wavePanel { "Waveform", "", Theme::blue };
    AMWaveView wave;
    class SourceInfo;
    std::unique_ptr<SourceInfo> info;
    struct Section { std::unique_ptr<ParamPanel> panel; float weight; };
    std::vector<Section> sections;
    // SAMPLE and GESTURE have dedicated panels instead of the generic sections.
    std::unique_ptr<SamplePanel> samplePanel;
    std::unique_ptr<GesturePanel> gesturePanel;
    int currentSource = -1;
    std::array<float, 1024> tapL {}, tapR {};
};

//==============================================================================
/** SHAPE page: the six Matter macros, materials and topology. */
class ShapePage : public juce::Component, private juce::Timer
{
public:
    explicit ShapePage (AntiMatrProcessor& p);
    ~ShapePage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    AntiMatrProcessor& processor;
    ParamPanel matter, materials, topology, response;
};

//==============================================================================
/** EVOLVE page: operators, operator detail and the gravity / scatter field. */
class EvolvePage : public juce::Component, private juce::Timer
{
public:
    explicit EvolvePage (AntiMatrProcessor& p);
    ~EvolvePage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    AntiMatrProcessor& processor;
    ParamPanel operators, bend, motion;
    AMPanel magnetPanel { "Magnet", "Alignment target", Theme::violet };
    std::unique_ptr<AMOptionList> magnetList;
    std::unique_ptr<juce::ParameterAttachment> magnetAttachment;
    AMPanel fieldPanel { "Field", "Gravity, scatter, crush & freeze", Theme::violet };
    AMXYPad field { "Gravity", "Scatter", Theme::violet };
    std::vector<std::unique_ptr<BoundControl>> fieldControls;   // crush & freeze, under the pad
    std::unique_ptr<juce::ParameterAttachment> fieldX, fieldY;
    std::array<std::unique_ptr<EvolvePanel::OperatorCell>, 4> cells;
    std::unique_ptr<juce::ParameterAttachment> selectedAttachment;
};

//==============================================================================
/** FRACTURE page: spectrum, fragment sequencer and every spectral control. */
class FracturePage : public juce::Component, private juce::Timer
{
public:
    explicit FracturePage (AntiMatrProcessor& p);
    ~FracturePage() override { stopTimer(); }
    void resized() override;

    /** Sequencer pattern as shown (mirrors the processor's FractureTable step gates). */
    std::vector<float> pattern() const { return steps.getSteps(); }

private:
    void timerCallback() override;
    void pushStepsToProcessor();
    void pullStepsFromProcessor();
    AntiMatrProcessor& processor;
    std::array<float, kMaxSequencerSteps> shownGates {};
    bool pushing = false;
    ParamPanel engine, sequencer, spectral;
    AMSpectrumView spectrum;
    SpectrumAnalyzer analyzer;
    std::array<float, SpectrumAnalyzer::kSize> tapL {}, tapR {};
    std::array<float, AMSpectrumView::kBands> bands {};
    AMStepEditor steps { Theme::magenta };
    std::unique_ptr<juce::ParameterAttachment> stepsAttachment;
    double playheadPhase = 0.0;
    uint64_t lastSampleTime = 0;
    int lastEngineStep = -1;
};

//==============================================================================
/** SPACE page: environment picker and the FX rack. */
class SpacePage : public juce::Component, private juce::Timer
{
public:
    explicit SpacePage (AntiMatrProcessor& p);
    ~SpacePage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    AntiMatrProcessor& processor;
    AMPanel spacePanel { "Space", "Place it anywhere", Theme::ivory };
    SpacePanel::SpacePicker picker;
    std::unique_ptr<juce::ParameterAttachment> typeAttachment;
    std::vector<std::unique_ptr<BoundControl>> macros;
    std::vector<std::unique_ptr<ParamPanel>> modules;
};

//==============================================================================
/** MOD page: routings, LFOs, envelopes, chaos generators, macros, amp and master. */
class ModPage : public juce::Component, private juce::Timer
{
public:
    explicit ModPage (AntiMatrProcessor& p);
    ~ModPage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    void showTab (int index);
    AntiMatrProcessor& processor;
    AMTabBar tabs { { "Routings", "LFO", "Envelopes", "Chaos", "Macros", "Amp & Master" }, Theme::amber,
                    { Icon::Grid, Icon::Lfo, Icon::Env, Icon::Chaos, Icon::Macro, Icon::Settings } };
    std::vector<std::vector<std::unique_ptr<ParamPanel>>> tabPanels;
    std::unique_ptr<ModRoutingPanel> routings;
    int current = 0;
};

} // namespace am::ui
