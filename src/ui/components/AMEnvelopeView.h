#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    The shape of an ADSR envelope, drawn from its own parameters.

    Attack, decay and release are seconds; sustain is 0..1 and curve bends
    every segment from logarithmic (-1) through linear (0) to exponential
    (+1). The horizontal scale is shared by the stages in proportion to their
    lengths, with a fixed sustain plateau in the middle, so two envelopes with
    the same settings always look the same and a long release always looks
    longer than a short one.

    Purely a display: it never touches a parameter.
*/
class AMEnvelopeView : public juce::Component
{
public:
    explicit AMEnvelopeView (juce::Colour accent);

    /** Times in seconds, sustain 0..1, curve -1..1. */
    void setEnvelope (float attackSeconds, float decaySeconds, float sustain, float releaseSeconds, float curve);
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    /** Activity 0..1 lifts the glow (e.g. voices playing). */
    void setActivity (float a) { if (std::abs (a - activity) > 0.03f) { activity = a; repaint(); } }

    void paint (juce::Graphics& g) override;

private:
    juce::Colour accent;
    float attack = 0.01f, decay = 0.2f, sustain = 0.7f, release = 0.3f, curve = 0.0f;
    float activity = 0.0f;
};

} // namespace am::ui
