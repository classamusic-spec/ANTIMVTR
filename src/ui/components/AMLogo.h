#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/** The ANTI-MATR wordmark + tagline drawn from geometry and typography. */
class AMLogo : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void setEnergy (float e) { energy = e; }
private:
    float energy = 0.0f;
};

} // namespace am::ui
