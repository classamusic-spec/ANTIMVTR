#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"

namespace am::ui
{

/**
    Vertical list of mutually exclusive options — one click to any of them.

    Used where a choice has enough entries that a `‹ VALUE ›` stepper would
    hide most of them (gesture models, magnet targets). The selected row wears
    a soft accent wash with a lit edge bar; hovering lifts a row. Row height,
    type size and radii all derive from the component's own bounds, and rows
    shrink their font before they clip.

    Optionally shows a one-line description under the list, which keeps the
    explanation attached to the thing it explains.
*/
class AMOptionList : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    AMOptionList (juce::StringArray items, juce::Colour accent);

    void setSelected (int index, juce::NotificationType notify = juce::sendNotification);
    int  getSelected() const noexcept { return selected; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }

    /** Per-option description shown under the list (empty entries hide it). */
    void setDescriptions (juce::StringArray lines) { descriptions = std::move (lines); repaint(); }

    /** Height of one row for the current bounds — used by owners that size the list to its content. */
    float rowHeight() const noexcept;
    /** Height the list needs for `count` rows plus its description area. */
    static int preferredHeight (int count, int rowHeight, bool withDescription) noexcept
    {
        return count * rowHeight + (withDescription ? juce::jmax (28, rowHeight * 2) : 0);
    }

    std::function<void (int)> onChange;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hovered = -1; repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override;

private:
    juce::Rectangle<float> listBounds() const;
    juce::Rectangle<float> descriptionBounds() const;
    int rowAt (juce::Point<int> p) const;

    juce::StringArray names, descriptions;
    juce::Colour accent;
    int selected = 0, hovered = -1;
    Eased lit;
    Animator anim { *this, { &lit }, 0.3f };
};

} // namespace am::ui
