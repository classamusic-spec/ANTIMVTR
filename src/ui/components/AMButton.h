#pragma once

#include "AMDrawing.h"
#include "AMIcons.h"

namespace am::ui
{

/** Flat text button with a subtle hover glow. Used for BROWSE / RANDOM / MUTATE etc. */
class AMButton : public juce::Button
{
public:
    explicit AMButton (const juce::String& text, juce::Colour accent = Theme::textPrimary);

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setOutlined (bool b) { outlined = b; repaint(); }
    void setIcon (std::optional<Icon> icon) { iconGlyph = icon; repaint(); }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

private:
    juce::Colour accent;
    bool outlined = false;
    std::optional<Icon> iconGlyph;
};

/** Round icon button (settings gear, chevrons). */
class AMIconButton : public juce::Button
{
public:
    AMIconButton (Icon icon, juce::Colour accent = Theme::textSecondary);
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setIcon (Icon i) { icon = i; repaint(); }
private:
    Icon icon;
    juce::Colour accent;
};

/**
    Segmented two-or-more state switch (SIMPLE / ADVANCED, OFF / ON, A / B).
    Emits onChange (index).
*/
class AMSegment : public juce::Component
{
public:
    explicit AMSegment (juce::StringArray items, juce::Colour accent = Theme::cyan);

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int getSelected() const noexcept { return selected; }
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
};

/** Horizontal slider with label on the left and value readout on the right (AMOUNT / SPEED). */
class AMSlider : public juce::Slider
{
public:
    explicit AMSlider (const juce::String& label, juce::Colour accent = Theme::violet);
    void paint (juce::Graphics& g) override;
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setLabel (const juce::String& l) { label = l; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void resized() override;

private:
    juce::String label;
    juce::Colour accent;
};

} // namespace am::ui
