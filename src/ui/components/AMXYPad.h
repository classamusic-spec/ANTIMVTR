#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMTooltip.h"

namespace am::ui
{

/**
    Two-dimensional pad: a glowing point on a dark inset field with faint
    grid lines, axis labels and readouts. Drag to move, double-click resets
    to the default position, shift-drag is fine. Emits onChange (x, y) with
    normalised 0..1 values; bind each axis with a juce::ParameterAttachment.
*/
class AMXYPad : public juce::Component,
                public juce::SettableTooltipClient
{
public:
    AMXYPad (const juce::String& xLabel, const juce::String& yLabel, juce::Colour accent = Theme::violet);

    void setPosition (float x, float y, juce::NotificationType notify = juce::sendNotification);
    float getX01() const noexcept { return px; }
    float getY01() const noexcept { return py; }
    void setDefault (float x, float y) { dx = x; dy = y; }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    /** Text shown in the readouts (defaults to two-decimal values). */
    std::function<juce::String (float)> formatX, formatY;
    /** Energy 0..1 brightens the point (e.g. audio level). */
    void setEnergy (float e) { if (std::abs (e - energy) > 0.02f) { energy = e; repaint(); } }

    std::function<void (float, float)> onChange;
    std::function<void()> onDragStart, onDragEnd;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { anim.animate (hover, 1.0f); }
    void mouseExit (const juce::MouseEvent&) override { anim.animate (hover, 0.0f); }

private:
    juce::Rectangle<float> field() const;
    /** Where the point may actually travel: the field inset by the marker radius so 0 and 1 are never clipped. */
    juce::Rectangle<float> plotArea() const;
    float markerRadius() const;
    void updateFromMouse (const juce::MouseEvent& e);
    juce::String xLabel, yLabel;
    juce::Colour accent;
    float px = 0.5f, py = 0.5f, dx = 0.5f, dy = 0.5f;
    float energy = 0.0f;
    bool dragging = false;
    juce::Point<float> dragOrigin, dragValueOrigin;
    Eased hover;
    Animator anim { *this, { &hover } };
    AMTooltip tip;
    static constexpr int kTrail = 24;
    std::array<juce::Point<float>, kTrail> trail {};
    int trailHead = 0, trailCount = 0;
};

} // namespace am::ui
