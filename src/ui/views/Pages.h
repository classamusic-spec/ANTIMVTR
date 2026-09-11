#pragma once

#include "Panels.h"
#include "ModRoutingPanel.h"
#include "SourcePanels.h"
#include "ui/components/AMTab.h"
#include "ui/components/AMXYPad.h"
#include "ui/components/AMStepEditor.h"
#include "ui/components/AMOptionList.h"
#include "ui/components/AMEnvelopeView.h"

namespace am::ui
{

/**
    Registry-driven panel: a titled AMPanel holding one bound control per
    parameter, laid out in a grid, with optional footer rows under it.

    Deep pages use one of these as the cluster on their right-hand side and
    swap its contents as the sidebar selection changes.
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
    /** Removes whatever the header is carrying (a module with no switch of its own). */
    void clearHeaderToggle();
    void setColumns (int c) { columns = c; resized(); }

    /** Replaces the panel's controls; the deep pages swap these per sidebar tab. */
    void setParams (std::vector<Param> params, int columns = 0, const juce::String& labelPrefix = {});
    /**
        Rows laid out under the main grid, one line each, evenly divided. This is
        what carries the stepper rows the reference puts under the knobs
        (TABLE / UNISON, then OCTAVE / SEMITONE / FINE).
    */
    void setFooterRows (std::vector<std::vector<Param>> rows, const juce::String& labelPrefix = {});
    /** The same, above the display — the FRACTURE mode selector sits here. */
    void setTopRows (std::vector<std::vector<Param>> rows, const juce::String& labelPrefix = {});
    /** Share of the content height the main grid keeps when there are footer rows. */
    void setGridFraction (float f) { gridFraction = juce::jlimit (0.2f, 1.0f, f); resized(); }

    /** Rows of controls this panel lays out (used to share height between panels). */
    int rowsNeeded() const noexcept
    {
        const int n = (int) controls.size();
        const int cols = columns > 0 ? columns : juce::jmax (1, n);
        return n == 0 ? 0 : (n + cols - 1) / cols;
    }
    bool hasDisplay() const noexcept { return display != nullptr; }
    /** Places a display (a curve, a scope) across the top `fraction` of the content area. */
    void setDisplay (juce::Component* c, float fraction) { display = c; displayFraction = fraction; if (c != nullptr) addAndMakeVisible (*c); resized(); }
    /** Draws the live modulation rings on every knob in the panel. */
    void refreshModRings (const ModulationSnapshot& s);
    void setHeroKnobs (bool hero);
    /** Per-parameter accent override. */
    void setAccentFor (Param p, juce::Colour c);

private:
    std::unique_ptr<BoundControl> makeControl (Param p, const juce::String& prefix);

    AntiMatrProcessor& processor;
    std::vector<std::unique_ptr<BoundControl>> controls;
    std::vector<std::unique_ptr<BoundControl>> footer, topControls;
    std::vector<int> footerRowSizes, topRowSizes;
    juce::String defaultPrefix;
    std::unique_ptr<BoundControl> headerToggle;
    std::unique_ptr<AMSegment> headerSwitch;
    std::unique_ptr<juce::ParameterAttachment> headerSwitchAttachment;
    juce::Component* display = nullptr;
    float displayFraction = 0.0f;
    float gridFraction = 0.58f;
    int columns = 0;
};

//==============================================================================
/**
    SOURCE page — the pill list of energies on the left, the wavetable as a
    luminous mesh on the dark screen in the middle with its table selector
    beneath it, and the source's controls on the right: two rows of three knobs
    over the stepper rows.

    SAMPLE and GESTURE keep their dedicated panels, which carry a display of
    their own (the sample waveform, the pressure x speed field) and so span the
    middle and right columns together.
*/
class SourcePage : public juce::Component, private juce::Timer
{
public:
    explicit SourcePage (AntiMatrProcessor& p);
    ~SourcePage() override;
    void resized() override;

private:
    void timerCallback() override;
    void rebuild (int source);
    void applyTab();
    static Param modeParam (int source);

    AntiMatrProcessor& processor;
    AMPanel sidebarPanel { "Source", "Choose your energy", Theme::blue };
    AMSidebar sidebar;
    std::unique_ptr<juce::ParameterAttachment> selectorAttachment;
    AMSegment tabs { { "Main", "Advanced" }, Theme::blue };
    std::unique_ptr<BoundControl> sourceModeControl;

    WaveMeshDisplay mesh;
    std::unique_ptr<AMChoice> tableSelector;
    std::unique_ptr<juce::ParameterAttachment> tableAttachment;

    std::unique_ptr<ParamPanel> cluster;
    std::unique_ptr<SamplePanel> samplePanel;
    std::unique_ptr<GesturePanel> gesturePanel;

    std::vector<Param> mainParams, advancedParams;
    std::vector<std::vector<Param>> mainRows;
    int currentSource = -1;
    bool advanced = false;
    std::array<float, 1024> tapL {}, tapR {};
};

//==============================================================================
/**
    SHAPE page — SIMPLE / ADVANCED / MATERIAL on the left, the material as a
    crystalline node lattice on the dark screen with the material blend selector
    beneath it, and two rows of three knobs on the right.
*/
class ShapePage : public juce::Component, private juce::Timer
{
public:
    explicit ShapePage (AntiMatrProcessor& p);
    ~ShapePage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    void showTab (int index);

    AntiMatrProcessor& processor;
    AMPanel sidebarPanel { "Shape", "Matter into sound", Theme::cyan };
    AMSidebar sidebar;
    LatticeDisplay lattice;
    AMPanel blendPanel { "Material Blend", "", Theme::cyan };
    std::unique_ptr<BoundControl> materialA, materialB;
    AMSlider blend { "Blend", Theme::cyan };
    std::unique_ptr<SliderAttachment> blendAttachment;
    std::unique_ptr<ParamPanel> cluster;
    int current = 0;
};

//==============================================================================
/**
    EVOLVE page — MAIN / ADVANCED / MOTION on the left, the deformation as a
    flowing ribbon surface on the dark screen, and the four operators with the
    AMOUNT and SPEED sliders beneath them on the right.
*/
class EvolvePage : public juce::Component, private juce::Timer
{
public:
    explicit EvolvePage (AntiMatrProcessor& p);
    ~EvolvePage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    void showTab (int index);
    void bindAmount (int op);

    AntiMatrProcessor& processor;
    AMPanel sidebarPanel { "Evolve", "Movement & change", Theme::violet };
    AMSidebar sidebar;
    RibbonDisplay ribbon;

    AMPanel operatorPanel { "Operators", "Bend, melt, tear & magnet", Theme::violet };
    std::array<std::unique_ptr<BoundControl>, 4> operatorKnobs;
    AMSlider amount { "Amount", Theme::violet };
    AMSlider speed { "Speed", Theme::violet };
    std::unique_ptr<SliderAttachment> amountAttachment, speedAttachment;
    std::unique_ptr<juce::ParameterAttachment> selectedAttachment;
    int selectedOperator = 0;

    std::unique_ptr<ParamPanel> cluster;              // ADVANCED / MOTION
    AMPanel magnetPanel { "Magnet", "Alignment target", Theme::violet };
    std::unique_ptr<AMOptionList> magnetList;
    std::unique_ptr<juce::ParameterAttachment> magnetAttachment;
    AMPanel fieldPanel { "Field", "Gravity & scatter", Theme::violet };
    AMXYPad field { "Gravity", "Scatter", Theme::violet };
    std::unique_ptr<juce::ParameterAttachment> fieldX, fieldY;
    int current = 0;
};

//==============================================================================
/**
    FRACTURE page — MAIN / SEQUENCER / FRAGMENTS on the left, the object
    shattered into a burst of shards on the dark screen (with the step editor
    under it on the SEQUENCER tab), and the mode selector, bar spectrum and four
    knobs on the right.
*/
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
    void showTab (int index);
    void pushStepsToProcessor();
    void pullStepsFromProcessor();

    AntiMatrProcessor& processor;
    std::array<float, kMaxSequencerSteps> shownGates {};
    bool pushing = false;

    AMPanel sidebarPanel { "Fracture", "New realities", Theme::magenta };
    AMSidebar sidebar;
    AMSegment onOff { { "Off", "On" }, Theme::magenta };
    std::unique_ptr<juce::ParameterAttachment> onAttachment;

    ShardDisplay shards;
    AMStepEditor steps { Theme::magenta };
    std::unique_ptr<juce::ParameterAttachment> stepsAttachment;

    std::unique_ptr<ParamPanel> cluster;
    AMSpectrumView spectrum;
    SpectrumAnalyzer analyzer;
    std::array<float, SpectrumAnalyzer::kSize> tapL {}, tapR {};
    std::array<float, AMSpectrumView::kBands> bands {};
    double playheadPhase = 0.0;
    uint64_t lastSampleTime = 0;
    int lastEngineStep = -1;
    int current = 0;
};

//==============================================================================
/**
    SPACE page — the list of spaces on the left, the space itself generated on
    the dark screen in the middle, and MIX / SIZE / TONE / FEEDBACK over the
    SPACE ENGINE rack on the right.
*/
class SpacePage : public juce::Component, private juce::Timer
{
public:
    explicit SpacePage (AntiMatrProcessor& p);
    ~SpacePage() override { stopTimer(); }
    void resized() override;

private:
    void timerCallback() override;
    void selectModule (int index);

    struct Module
    {
        juce::String name, prefix;
        std::optional<Param> power;
        std::vector<Param> params;
        std::unique_ptr<AMModuleTile> tile;
        std::unique_ptr<juce::ParameterAttachment> attachment;
    };

    AntiMatrProcessor& processor;
    AMPanel sidebarPanel { "Space", "Place it anywhere", Theme::ivory };
    AMSidebar sidebar;
    std::unique_ptr<juce::ParameterAttachment> typeAttachment;

    SpaceDisplay space;

    AMPanel macroPanel { "Macros", "Mix, size, tone & feedback", Theme::ivory };
    std::vector<std::unique_ptr<BoundControl>> macros;
    AMPanel enginePanel { "Space Engine", "Modules in the chain", Theme::ivory };
    std::vector<std::unique_ptr<Module>> modules;
    std::unique_ptr<ParamPanel> moduleControls;
    int selectedModule = 0;
};

//==============================================================================
/** MOD page: routings, LFOs, envelopes, chaos generators, macros, amp and master. */
class ModPage : public juce::Component, private juce::Timer
{
public:
    explicit ModPage (AntiMatrProcessor& p);
    ~ModPage() override { stopTimer(); }
    void resized() override;

    /** Selects one of the tabs (ROUTINGS, LFO, ENVELOPES, CHAOS, MACROS, AMP & MASTER). */
    void showTab (int index);

private:
    void timerCallback() override;
    AntiMatrProcessor& processor;
    AMTabBar tabs { { "Routings", "LFO", "Envelopes", "Chaos", "Macros", "Amp & Master" }, Theme::amber,
                    { Icon::Grid, Icon::Lfo, Icon::Env, Icon::Chaos, Icon::Macro, Icon::Settings } };
    std::vector<std::vector<std::unique_ptr<ParamPanel>>> tabPanels;
    std::unique_ptr<ModRoutingPanel> routings;
    std::vector<std::unique_ptr<AMEnvelopeView>> envelopeViews;   // one per envelope panel, plus AMP
    int current = 0;
};

} // namespace am::ui
