#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/**
    The ANTI-MATR wordmark: the first A drawn as a stylised Λ with a small
    glowing particle, the rest set in the display face, and the tagline in
    tiny tracked caps beneath.
*/
class AMLogo : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void setEnergy (float e) { if (std::abs (e - energy) > 0.02f) { energy = e; repaint(); } }
    /** Hides the tagline (compact placements). */
    void setShowTagline (bool b) { tagline = b; repaint(); }

    /** Draws the wordmark into any area (shared with other views). */
    static void drawWordmark (juce::Graphics& g, juce::Rectangle<float> area, float energy, bool withTagline, juce::Justification just = juce::Justification::centredLeft);

private:
    float energy = 0.0f;
    bool tagline = true;
};

} // namespace am::ui
