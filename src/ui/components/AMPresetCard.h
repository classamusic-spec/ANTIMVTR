#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"

namespace am::ui
{

/**
    Preset card for the browser: procedural art derived from the preset's
    category and name, the name in the display face, category caption and
    tag chips. The current preset carries a glowing accent frame.
*/
class AMPresetCard : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    AMPresetCard();

    void setPreset (int index, const juce::String& name, const juce::String& category, const juce::StringArray& tags);
    void setCurrent (bool on);
    bool isCurrent() const noexcept { return current; }
    int getIndex() const noexcept { return index; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }

    std::function<void (int)> onClick;          ///< preset index
    std::function<void (int)> onDoubleClick;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { anim.animate (hover, 1.0f); }
    void mouseExit (const juce::MouseEvent&) override { anim.animate (hover, 0.0f); }

    /** Draws the category art (shared with other views). */
    static void drawArt (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& category, int seed, float lit, float corner);

private:
    int index = -1;
    juce::String name, category;
    juce::StringArray tags;
    bool current = false;
    juce::Colour accent = Theme::blue;
    Eased hover, lit;
    Animator anim { *this, { &hover, &lit } };
};

} // namespace am::ui
