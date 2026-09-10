#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    Modulation ring drawn around a rotary control. Pure data API: the owner
    feeds the base value, the modulation range and the current (modulated)
    position, all normalised 0..1. Nothing is drawn until the ring has a
    range or the current position differs from the base value, so a knob
    without modulation looks exactly as before.

    The ring lives on its own orbit *outside* the knob's value arc, separated
    by a dark moat, and is always drawn in the modulation accent (amber) — a
    modulated knob is therefore unmistakable whatever its section colour. When
    more than one source reaches the same destination the ring carries a small
    count badge so the stack is visible at a glance.

    Usable stand-alone (place it over any rotary control and give it the
    same rotary angles) or through AMKnob::modRing().
*/
class AMModRing : public juce::Component
{
public:
    AMModRing();

    void setAngles (float startRadians, float endRadians);
    void setAccent (juce::Colour c) { accent = c; repaint(); }

    /** Base (unmodulated) value 0..1. */
    void setBase (float normalised);
    /** Modulation range 0..1 (absolute positions, lo <= hi). Pass equal values to hide the range. */
    void setRange (float lo, float hi);
    /** Current modulated position 0..1. */
    void setCurrent (float normalised);
    /** How many enabled routings reach this destination (>= 2 shows the count badge). */
    void setSourceCount (int n);
    /** Clears range, current position and the source count. */
    void clear();

    float getBase() const noexcept { return base; }
    float getCurrent() const noexcept { return current; }
    int  getSourceCount() const noexcept { return sourceCount; }
    bool isActive() const noexcept;

    /** Draws the ring into an arbitrary graphics context (used by the knob). */
    void draw (juce::Graphics& g, juce::Rectangle<float> ringBounds, float strokeWidth) const;

    void paint (juce::Graphics& g) override;

private:
    void drawCountBadge (juce::Graphics& g, juce::Rectangle<float> ringBounds, float strokeWidth) const;

    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle   = juce::MathConstants<float>::pi * 2.75f;
    float base = 0.0f, rangeLo = 0.0f, rangeHi = 0.0f, current = 0.0f;
    bool hasRange = false, hasCurrent = false;
    int sourceCount = 0;
    juce::Colour accent { 0xffffb46b };
};

} // namespace am::ui
