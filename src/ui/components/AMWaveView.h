#pragma once

#include "AMDrawing.h"

namespace am::ui
{

/** Inset waveform display with a glowing trace and optional prev/next arrows and caption. */
class AMWaveView : public juce::Component
{
public:
    AMWaveView();

    /** Copies samples to display (any count; the view resamples). */
    void setSamples (const float* data, int numSamples);
    void setAccent (juce::Colour c) { accent = c; }
    void setCaption (const juce::String& text) { caption = text; }
    void setEnergy (float e) { energy = e; }

    std::function<void (int)> onArrow;   ///< -1 / +1 when the side chevrons are clicked

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    std::vector<float> samples;
    juce::Colour accent = Theme::blue;
    juce::String caption;
    float energy = 0.0f;
};

/**
    Layered spectrum display in the FRACTURE style: translucent blue and
    magenta bands with vertical fragment grid lines.
*/
class AMSpectrumView : public juce::Component
{
public:
    static constexpr int kBands = 64;

    void setMagnitudes (const float* mags, int count);   ///< 0..1 per band
    void setFragmentCount (int n) { fragments = juce::jlimit (1, 64, n); }
    void setActivity (float a) { activity = a; }
    void paint (juce::Graphics& g) override;

private:
    std::array<float, kBands> bands {};
    std::array<float, kBands> slow {};
    int fragments = 16;
    float activity = 0.0f;
};

} // namespace am::ui
