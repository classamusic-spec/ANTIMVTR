#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMIcons.h"

namespace am::ui
{

/** Flat text button with a subtle hover glow. Used for BROWSE / RANDOM / MUTATE etc. */
class AMButton : public juce::Button
{
public:
    explicit AMButton (const juce::String& text, juce::Colour accent = Theme::textPrimary);

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    /** Outlined pill style (used in overlays and chips). */
    void setOutlined (bool b) { outlined = b; repaint(); }
    /** Filled accent style (primary actions). */
    void setFilled (bool b) { filled = b; repaint(); }
    void setIcon (std::optional<Icon> icon) { iconGlyph = icon; repaint(); }
    /** Small chip look (tag chips in the browser). */
    void setChip (bool b) { chip = b; repaint(); }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Colour accent;
    bool outlined = false, filled = false, chip = false;
    std::optional<Icon> iconGlyph;
    Eased hover;
    Animator anim { *this, { &hover } };
};

/** Round icon button (settings gear, chevrons, close). */
class AMIconButton : public juce::Button
{
public:
    AMIconButton (Icon icon, juce::Colour accent = Theme::textSecondary);
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setIcon (Icon i) { icon = i; repaint(); }
    /** Draws a faint circular outline (used in the preset pill). */
    void setOutlined (bool b) { outlined = b; repaint(); }
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
private:
    Icon icon;
    juce::Colour accent;
    bool outlined = false;
    Eased hover;
    Animator anim { *this, { &hover } };
};

/**
    Segmented two-or-more state switch (SIMPLE / ADVANCED, OFF / ON, A / B).
    The lit thumb glides between cells. Emits onChange (index).
*/
class AMSegment : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    explicit AMSegment (juce::StringArray items, juce::Colour accent = Theme::cyan);

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int getSelected() const noexcept { return selected; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    std::function<void (int)> onChange;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    int indexAt (juce::Point<int> p) const;
    juce::StringArray items;
    juce::Colour accent;
    int selected = 0;
    int hover = -1;
    Eased thumb;
    Animator anim { *this, { &thumb }, 0.32f };
};

/** Horizontal slider with label on the left and value readout on the right (AMOUNT / SPEED). */
class AMSlider : public juce::Slider
{
public:
    explicit AMSlider (const juce::String& label, juce::Colour accent = Theme::violet);
    void paint (juce::Graphics& g) override;
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setLabel (const juce::String& l) { label = l.toUpperCase(); repaint(); }
    /** Hides the label column (compact placements such as the nav bar). */
    void setShowLabel (bool b) { showLabel = b; repaint(); }
    void setShowValue (bool b) { showValue = b; repaint(); }
    /** Bipolar sliders fill from the centre of the track instead of the left end. */
    void setBipolar (bool b) { bipolar = b; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::String label;
    juce::Colour accent;
    bool showLabel = true, showValue = true, bipolar = false;
    Eased hover;
    Animator anim { *this, { &hover } };
};

} // namespace am::ui
