#pragma once

#include "Controls.h"
#include "ui/components/AMSourceSelector.h"
#include "ui/components/AMWaveView.h"
#include "ui/components/AMSpaceArt.h"
#include "ui/components/AMIcons.h"
#include "ui/visualizers/SpectrumAnalyzer.h"

namespace am::ui
{

//==============================================================================
/**
    The selector column shared by the SOURCE, SHAPE, EVOLVE, FRACTURE and SPACE
    pages: a vertical list of pills, each a small glyph and a label. The selected
    pill is a raised white slab with an accent bar down its left edge and a soft
    shadow under it; the rest sit flush and quiet.

    TEMPORARY — the chassis owns the reusable sidebar pill primitive. This is a
    local stand-in so the pages can be laid out to the reference before it lands,
    and the two want reconciling at merge.
*/
class AMSidebar : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    struct Item
    {
        juce::String label;
        Icon icon = Icon::Grid;
        juce::String caption;                  ///< optional second line
        juce::Colour accent = juce::Colour();  ///< transparent = use the list's accent
    };

    AMSidebar (std::vector<Item> entries, juce::Colour accentColour);

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int  getSelected() const noexcept { return selected; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    /** Accent of a row (its own, or the list's). */
    juce::Colour accentFor (int index) const noexcept;

    /** Height the list wants for its rows; owners centre it when the column is taller. */
    int preferredHeight (int width) const noexcept;

    std::function<void (int)> onChange;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hovered = -1; repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

private:
    juce::Rectangle<float> pillBounds (int index) const;
    float rowHeight() const noexcept;
    int rowAt (juce::Point<int> p) const;

    std::vector<Item> items;
    juce::Colour accent;
    int selected = 0, hovered = -1;
};

//==============================================================================
/**
    The dark screen at the centre of a deep page.

    A near-black recess cut into the pale chassis carrying luminous procedural
    line art — the one dark element on the page, and the contrast that carries
    the design. Subclasses draw the art; everything else (the recess, the inner
    shadow, the vignette, the glass and the lettering) is shared.
*/
class PageDisplay : public juce::Component
{
public:
    explicit PageDisplay (juce::Colour accentColour, const juce::String& titleText = {})
        : accent (accentColour), title (titleText.toUpperCase()) {}

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setTitle (const juce::String& t) { title = t.toUpperCase(); repaint(); }
    void setCaption (const juce::String& c) { if (c.toUpperCase() != caption) { caption = c.toUpperCase(); repaint(); } }
    void setEnergy (float e) { energy = juce::jlimit (0.0f, 1.0f, e); }
    /** Advances the art's clock; the page's timer calls this and repaints. */
    void advance (float seconds) { phase += seconds; repaint(); }

    void paint (juce::Graphics& g) override;

    /** Corner radius of the screen, derived from its own bounds. */
    float corner() const noexcept;

protected:
    /** Draws the art inside the (already clipped) screen. */
    virtual void paintArt (juce::Graphics& g, juce::Rectangle<float> area) = 0;
    /** True when the art draws its own recess and glass (SpaceArt does). */
    virtual bool paintsOwnGround() const { return false; }

    juce::Colour accent;
    juce::String title, caption;
    float phase = 0.0f, energy = 0.0f;
};

//==============================================================================
/** SOURCE — the wavetable as a luminous mesh of stacked frames seen in perspective. */
class WaveMeshDisplay : public PageDisplay
{
public:
    WaveMeshDisplay() : PageDisplay (Theme::blue) {}
    /** Table (or mode) index and the three shaping values, 0..1. */
    void setShape (int tableIndex, float pos, float scanAmount, float morphAmount);

protected:
    void paintArt (juce::Graphics& g, juce::Rectangle<float> area) override;

private:
    int table = 0;
    float position = 0.2f, scan = 0.0f, morph = 0.0f;
};

//==============================================================================
/** SHAPE — the material as a crystalline lattice of nodes and couplings. */
class LatticeDisplay : public PageDisplay
{
public:
    LatticeDisplay() : PageDisplay (Theme::cyan) {}
    void setMatter (float d, float f, float m, float t, float s, int topologyIndex, int seedValue);

protected:
    void paintArt (juce::Graphics& g, juce::Rectangle<float> area) override;

private:
    float density = 0.5f, form = 0.3f, mass = 0.4f, tension = 0.5f, surface = 0.2f;
    int topology = 2, seed = 7;
};

//==============================================================================
/** EVOLVE — the deformation as a flowing ribbon surface. */
class RibbonDisplay : public PageDisplay
{
public:
    RibbonDisplay() : PageDisplay (Theme::violet) {}
    void setOperators (float b, float m, float t, float mag, float sp);

protected:
    void paintArt (juce::Graphics& g, juce::Rectangle<float> area) override;

private:
    float bend = 0.0f, melt = 0.0f, tear = 0.0f, magnet = 0.0f, speed = 0.3f;
};

//==============================================================================
/** FRACTURE — the object shattered into a burst of shards. */
class ShardDisplay : public PageDisplay
{
public:
    ShardDisplay() : PageDisplay (Theme::magenta) {}
    void setFracture (int frags, float amt, float spr, float rnd, bool isOn, int seedValue);

protected:
    void paintArt (juce::Graphics& g, juce::Rectangle<float> area) override;

private:
    int fragments = 16, seed = 11;
    float amount = 0.5f, spread = 0.3f, random = 0.2f;
    bool on = false;
};

//==============================================================================
/** SPACE — the environment itself, generated by SpaceArt (which draws its own recess). */
class SpaceDisplay : public PageDisplay
{
public:
    SpaceDisplay() : PageDisplay (Theme::ivory) {}
    void setType (int t) { type = juce::jlimit (0, SpaceArt::kNumTypes - 1, t); repaint(); }
    int getType() const noexcept { return type; }

protected:
    void paintArt (juce::Graphics& g, juce::Rectangle<float> area) override;
    bool paintsOwnGround() const override { return true; }

private:
    int type = 0;
};

//==============================================================================
/**
    One module of the SPACE ENGINE rack: a small tile with a glyph and a label.

    Two targets in one tile, the way a rack module works: the power pip in its
    top-right corner switches the module on and off, the rest of the tile selects
    it so its parameters show underneath.
*/
class AMModuleTile : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    AMModuleTile (const juce::String& text, Icon icon, juce::Colour accentColour);

    void setPowered (bool on) { if (on != powered) { powered = on; repaint(); } }
    bool isPowered() const noexcept { return powered; }
    void setSelected (bool s) { if (s != selected) { selected = s; repaint(); } }
    /** A module with no on/off parameter of its own (EQ) hides the pip. */
    void setHasPower (bool b) { hasPower = b; repaint(); }

    std::function<void()> onSelect;
    std::function<void (bool)> onPower;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hoverPip = false; hovered = false; repaint(); }
    void mouseEnter (const juce::MouseEvent&) override { hovered = true; repaint(); }

private:
    juce::Rectangle<float> pipBounds() const;
    juce::String label;
    Icon glyph;
    juce::Colour accent;
    bool powered = false, selected = false, hasPower = true, hovered = false, hoverPip = false;
};

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
    LatticeDisplay lattice;
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
