#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"
#include "AMTooltip.h"

namespace am::ui
{

/**
    Step sequencer editor: 1–32 steps, drag to draw values, a glowing
    playhead column, hover highlight and a right-click menu with pattern
    tools. Pure data API: the owner reads/writes the step values and moves
    the playhead; nothing here talks to the engine.
*/
class AMStepEditor : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    static constexpr int kMaxSteps = 32;

    explicit AMStepEditor (juce::Colour accent = Theme::magenta);

    void setNumSteps (int n);
    int getNumSteps() const noexcept { return numSteps; }
    void setStep (int index, float value, juce::NotificationType notify = juce::sendNotification);
    float getStep (int index) const noexcept { return steps[(size_t) juce::jlimit (0, kMaxSteps - 1, index)]; }
    void setSteps (const std::vector<float>& values, juce::NotificationType notify = juce::sendNotification);
    std::vector<float> getSteps() const;
    /** Playhead position in steps (fractional allowed); < 0 hides it. */
    void setPlayhead (float position);
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    /** Values are bipolar (-1..1 drawn from the centre) instead of 0..1. */
    void setBipolar (bool b) { bipolar = b; repaint(); }

    std::function<void (int, float)> onStepChanged;
    std::function<void()> onPatternChanged;   ///< after menu tools / bulk edits

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hoverStep = -1; repaint(); }
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    juce::Rectangle<float> field() const;
    int stepAt (float x) const;
    float valueAt (float y) const;
    void paintStep (const juce::MouseEvent& e);
    void showMenu();
    void applyPattern (int id);

    std::array<float, kMaxSteps> steps {};
    int numSteps = 8;
    float playhead = -1.0f;
    int hoverStep = -1, lastPaintedStep = -1;
    float lastPaintedValue = 0.0f;
    bool bipolar = false;
    bool dragging = false;
    juce::Colour accent;
    AMTooltip tip;
};

} // namespace am::ui
