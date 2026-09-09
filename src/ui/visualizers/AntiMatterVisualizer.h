#pragma once

#include "ui/components/AMDrawing.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am::ui
{

/**
    THE ANTI-MATTER OBJECT — the product's visual signature.

    Procedurally rendered with JUCE Graphics (Phase 1): layered translucent
    polar blobs, a dark core, glowing filaments, orbiting fragments and a
    ring of node markers. Everything reacts to the engine's
    VisualStateSnapshot: Density → complexity, Form → geometric organisation,
    Mass → inertia, Tension → stretch, Decay → motion persistence,
    Surface → detail, Bend → deformation, Melt → diffusion, Tear →
    separation, Magnet → alignment, Fracture → shards, Space → halo,
    amplitude → pulse, pitch → structure.
*/
class AntiMatterVisualizer : public juce::Component,
                             private juce::Timer
{
public:
    explicit AntiMatterVisualizer (Diagnostics& diagnostics);
    ~AntiMatterVisualizer() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    /** Frame rate control: 60 by default, 30 when the host is heavy. */
    void setTargetFrameRate (int fps);

    /** Latest snapshot the visualizer is showing (for tests / tools). */
    const VisualStateSnapshot& state() const noexcept { return smooth; }

private:
    void timerCallback() override;
    void integrate (float dt);
    juce::Path blobPath (juce::Point<float> centre, float radius, float stretchX, float stretchY, float rotation,
                         const float* harmonics, int numHarmonics, float phaseOffset) const;
    void drawBackground (juce::Graphics& g, juce::Rectangle<float> area);
    void drawObject (juce::Graphics& g, juce::Rectangle<float> area);
    void drawFragments (juce::Graphics& g, juce::Rectangle<float> area);
    void drawNodes (juce::Graphics& g, juce::Rectangle<float> area);
    void drawCaptions (juce::Graphics& g, juce::Rectangle<float> area);

    Diagnostics& diag;
    VisualStateSnapshot latest;
    VisualStateSnapshot smooth;   // eased values for inertia
    float time = 0.0f;
    float pulse = 0.0f;
    float energyEnvelope = 0.0f;
    int   fps = 60;

    struct Fragment { float angle, radius, size, speed, spin, phase; int sides; };
    std::vector<Fragment> fragments;
    struct Star { float x, y, size, twinkle; };
    std::vector<Star> stars;
};

} // namespace am::ui
