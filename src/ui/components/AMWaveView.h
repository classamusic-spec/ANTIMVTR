#pragma once

#include "AMDrawing.h"
#include "AMAnimator.h"

namespace am::ui
{

/** Inset waveform display with a glowing trace, prev/next chevrons and a caption. */
class AMWaveView : public juce::Component
{
public:
    AMWaveView();

    /** Copies samples to display (any count; the view resamples). */
    void setSamples (const float* data, int numSamples);
    void setAccent (juce::Colour c) { accent = c; }
    void setCaption (const juce::String& text) { caption = text.toUpperCase(); repaint(); }
    void setEnergy (float e) { energy = e; }
    void setShowArrows (bool b) { arrows = b; repaint(); }

    std::function<void (int)> onArrow;   ///< -1 / +1 when the side chevrons are clicked

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hoverSide = 0; repaint(); }

private:
    int sideAt (juce::Point<int> p) const;
    std::vector<float> samples;
    juce::Colour accent = Theme::blue;
    juce::String caption;
    float energy = 0.0f;
    bool arrows = true;
    int hoverSide = 0;
    juce::Path trace, fill;
};

/**
    Layered spectrum display in the FRACTURE style: translucent blue,
    violet and magenta mountains with vertical fragment grid lines.
*/
class AMSpectrumView : public juce::Component
{
public:
    static constexpr int kBands = 64;

    AMSpectrumView();
    void setMagnitudes (const float* mags, int count);   ///< 0..1 per band
    void setFragmentCount (int n) { fragments = juce::jlimit (1, 64, n); }
    void setActivity (float a) { activity = a; }
    void setAccent (juce::Colour c) { accent = c; }
    void paint (juce::Graphics& g) override;

private:
    void buildMountain (juce::Path& p, const std::array<float, kBands>& src, juce::Rectangle<float> inner, float scale) const;
    std::array<float, kBands> bands {}, slow {}, ghost {};
    int fragments = 16;
    float activity = 0.0f;
    juce::Colour accent = Theme::magenta;
    juce::Path live, hold, haze;
};

} // namespace am::ui
