#pragma once

#include "ui/components/AMDrawing.h"
#include "dev/diagnostics/Diagnostics.h"
#include "ValueNoise.h"
#include "Iridescence.h"
#include "ObjectField.h"
#include "ParticleSystem.h"

namespace am::ui
{

/**
    THE ANTI-MATTER OBJECT — the product's visual signature.

    Procedurally rendered with JUCE Graphics (Phase 1 software path). Layers,
    back to front:

      1. Environment: Space-tinted halo, octave dial, drifting plasma aura
         (cached backdrop image, re-rendered every few frames)
      2. Starfield (live twinkle) and far crystalline fragments (parallax)
      3. Body: outer glow → dark liquid-glass volume → Fresnel edge glow →
         nested refraction shells → energy filaments → void core with
         gravitational-lens ring and Einstein arcs → specular highlight →
         iridescent Fresnel rim (blue → violet → magenta → cyan → ivory)
         → fracture cracks → surface sparkles
      4. Tear: a second lobe with taffy strands between the two
      5. Near fragments (with Decay motion streaks), energy sparks
      6. Node ring: frequency → angle (log scale), energy → size/brightness,
         cluster → colour; fundamental marker
      7. Captions (INHALE/IDEA, EXHALE/EVOLVE, wordmark)

    Everything reads VisualStateSnapshot only, eased with Mass-dependent
    inertia. Paint time is measured every frame; when the running average
    exceeds ~8 ms the quality level drops (fewer segments, shells,
    filaments, particles, slower backdrop refresh) and recovers when there
    is headroom. No per-frame heap allocation on our side: paths, outlines
    and particles are preallocated and reused.
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

    /** Latest snapshot the visualizer is showing (eased values; for tests / tools). */
    const VisualStateSnapshot& state() const noexcept { return smooth; }

    /** Running average of paint() duration in milliseconds. */
    float lastPaintMs() const noexcept { return paintMsAverage; }

    /** Current adaptive quality level: 0 = full, 1 = reduced, 2 = minimal. */
    int qualityLevel() const noexcept { return quality; }

private:
    struct Frame;                      // per-paint derived geometry (see .cpp)

    void timerCallback() override;
    void integrate (float dt);
    void updateQuality();

    void drawBackdrop (juce::Graphics& g, const Frame& f);
    void renderBackdrop (juce::Graphics& g, const Frame& f);
    void drawStars (juce::Graphics& g, const Frame& f);
    void drawFragments (juce::Graphics& g, const Frame& f, bool nearLayer);
    void drawSparks (juce::Graphics& g, const Frame& f);
    void drawLobe (juce::Graphics& g, const Frame& f, int lobe);
    void drawShells (juce::Graphics& g, const Frame& f, int lobe);
    void drawFilaments (juce::Graphics& g, const Frame& f, int lobe);
    void drawCore (juce::Graphics& g, const Frame& f, int lobe);
    void drawRim (juce::Graphics& g, const Frame& f, int lobe);
    void drawCracks (juce::Graphics& g, const Frame& f, int lobe);
    void drawStrands (juce::Graphics& g, const Frame& f);
    void drawNodes (juce::Graphics& g, const Frame& f);
    void drawCaptions (juce::Graphics& g, juce::Rectangle<float> area);

    Diagnostics& diag;
    VisualStateSnapshot latest;
    VisualStateSnapshot smooth;        // eased values for inertia

    // Animation state
    float time = 0.0f;
    float rotation = 0.0f;             // structural rotation of the object
    float hueDrift = 0.0f;             // iridescence cycling
    float pulse = 0.0f;                // audio level, fast attack / Decay release
    float energy = 0.0f;               // note envelope, eased
    float life = 0.0f;                 // 0 idle → 1 playing
    float pitchU = 0.35f;              // log-pitch 0..1
    float fracture = 0.0f;             // eased fracture activity (0 when off)
    int   fps = 60;
    int   frameCounter = 0;

    // Adaptive quality / profiling
    int   quality = 0;
    float paintMsAverage = 0.0f;
    int   framesSinceQualityChange = 0;
    bool  profileToStderr = false;
    static constexpr int kLayers = 17;
    std::array<double, kLayers> frameLayerMs {}, layerMsAverage {};
    juce::int64 lastTick = 0;
    void mark (int layer) noexcept;    // accumulates time since the previous mark into a layer bucket

    // Procedural sources
    ValueNoise     noise;
    Iridescence    iridescence;
    ObjectField    field;
    ParticleSystem particles;

    // Preallocated geometry
    static constexpr int kMaxShells = 4;
    std::array<ObjectField::Outline, 2> lobes;
    std::array<std::array<ObjectField::Outline, kMaxShells>, 2> shells;
    juce::Path bodyPath, shellPath, quad, curve, glyph, ringPath;

    // Cached backdrop (halo + aura + outer glow): rendered at half resolution
    // every few frames, upscaled once into a full-resolution image, blitted each frame.
    juce::Image backdropSmall, backdrop;
    int   backdropAge = 1000;
    float backdropScale = 1.0f;
    void upscaleBackdrop() noexcept;
};

} // namespace am::ui
