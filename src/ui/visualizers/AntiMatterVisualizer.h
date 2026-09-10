#pragma once

#include "ui/components/AMDrawing.h"
#include "dev/diagnostics/Diagnostics.h"
#include "ValueNoise.h"
#include "Iridescence.h"
#include "LiquidOrganism.h"
#include "ParticleSystem.h"
#include "Porthole.h"

namespace am::ui
{

/**
    THE CENTRE — a glass porthole with the ANTI-MATTER object alive behind it.

    This is the identity of the product (VISUAL_SPEC §5). Everything is drawn
    procedurally from `getLocalBounds()`: no bitmaps, no assets, nothing loaded
    from disk. Layers, back to front:

      1. The blue arc spilling from beneath the plinth onto the panel.
      2. The glass well: a deep near-black bowl with a faint Space-tinted haze.
      3. THE OBJECT, clipped to the glass — a liquid-light organism:
           · bubbles and sparkles on the far side of the volume,
           · ribbon spans whose mean depth is behind the core, back to front,
           · the dark irregular organic core with its violet rim,
           · ribbon spans in front of the core, each casting a faint shadow,
           · near sparkles and bubbles.
         Ribbons are wide tapering strips of liquid light — a molten core with a
         translucent bloom either side — that shift hue along their length and
         advect through a slowly evolving 3D flow field (see LiquidOrganism).
      4. The glass itself: a broad diagonal specular sweep, a crescent under the
         top of the bezel, thickness darkening to the lower right, a chromatic
         fringe at the extreme edge and slow drifting smudges.
      5. The bezel: brushed gunmetal segmented into plates with seams and bolt
         heads, an outer shadow onto the panel and an inner shadow onto the glass.
      6. The two warm amber-white status lamps at nine and three o'clock.
      7. The machined plinth with its chrome rim and engraved wordmark.
      8. The flank captions (INHALE / IDEA, EXHALE / EVOLVE).

    The hardware (bezel, plinth, glass highlights) is static geometry, so it is
    rendered once into cached images and blitted; only the object, the lamp bloom
    and the plinth glow are redrawn each frame. Paint time is measured every
    frame and the quality level drops when it runs long.

    Reads VisualStateSnapshot only, eased with Mass-dependent inertia. Never
    blocks and never touches the audio thread.
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

    /** Frame rate control: 30 by default, which is what the object is budgeted for. */
    void setTargetFrameRate (int fps);

    /** Latest snapshot the visualizer is showing (eased values; for tests / tools). */
    const VisualStateSnapshot& state() const noexcept { return smooth; }

    /** Running average of paint() duration in milliseconds. */
    float lastPaintMs() const noexcept { return paintMsAverage; }

    /** Current adaptive quality level: 0 = full, 1 = reduced, 2 = minimal. */
    int qualityLevel() const noexcept { return quality; }

private:
    /** Everything derived once per paint from the eased state and the component bounds. */
    struct Frame
    {
        juce::Rectangle<float> bounds;
        PortholeLayout port;
        juce::Point<float> centre;      ///< centre of the object (== centre of the porthole)
        float R = 1.0f;                 ///< pixel radius of the implicit sphere
        float unit = 1.0f;              ///< one reference pixel (outerR / 186)
        float coreR = 0.3f;             ///< core radius, object units
        float breathe = 1.0f;
        juce::Colour space;

        // this frame's budget (quality level capped by the porthole's size)
        int  maxRibbons = 22, maxSamples = 44, maxSparkles = 130, maxBubbles = 18;
        bool shadows = true, bloom = true, smudges = true;

        // the object's rotation (yaw then a fixed pitch), precomputed
        float cosYaw = 1.0f, sinYaw = 0.0f, cosPitch = 1.0f, sinPitch = 0.0f;

        LiquidOrganism::Params p;
        float pulse = 0.0f, energy = 0.0f, life = 0.0f, fracture = 0.0f, hue = 0.0f;
        float surfaceRough = 0.0f, spaceActivity = 0.0f, freezeMix = 0.0f;
    };

    /** One ribbon sample after projection into the component. */
    struct Projected
    {
        float x = 0.0f, y = 0.0f;       ///< pixels
        float z = 0.0f;                 ///< rotated depth, −1 far .. +1 near
        float scale = 1.0f;             ///< perspective scale
        float nx = 0.0f, ny = 0.0f;     ///< screen-space normal of the ribbon
        float w0 = 0.0f, w1 = 0.0f;     ///< half-width in pixels of each edge (SURFACE roughens them apart)
    };

    void timerCallback() override;
    void integrate (float dt);
    void updateQuality();
    void mark (int layer) noexcept;

    // ---- the object
    void buildOrganism (const Frame& f);
    void drawWell (juce::Graphics& g, const Frame& f);
    void renderWell (juce::Graphics& g, const Frame& f);
    void drawSpanRange (juce::Graphics& g, const Frame& f, int from, int to);
    void drawRibbonSpan (juce::Graphics& g, const Frame& f, const RibbonSpan& span);
    void drawCore (juce::Graphics& g, const Frame& f);
    void drawSparkles (juce::Graphics& g, const Frame& f, bool front);
    void drawBubbles (juce::Graphics& g, const Frame& f, bool front);
    void buildSpanPath (juce::Path& path, const RibbonSpan& span, float widthScale,
                        float offsetX, float offsetY) const;
    void computeBudget();

    // ---- the hardware (Porthole.cpp)
    void refreshHardware (const Frame& f, float deviceScale);
    void renderBezel (juce::Graphics& g, const Frame& f);
    void renderGlass (juce::Graphics& g, const Frame& f);
    void renderPlinth (juce::Graphics& g, const Frame& f);
    void drawPlinthGlow (juce::Graphics& g, const Frame& f);
    void drawLamps (juce::Graphics& g, const Frame& f);
    void drawSmudges (juce::Graphics& g, const Frame& f);
    void drawCaptions (juce::Graphics& g, const Frame& f);

    Diagnostics& diag;
    VisualStateSnapshot latest;
    VisualStateSnapshot smooth;         // eased values for inertia

    // Animation state
    float time = 0.0f;                  // wall clock
    float flowTime = 0.0f;              // flow-field clock — held still by Freeze
    float rotation = 0.0f;              // yaw of the organism
    float hueDrift = 0.0f;              // slow travel along the ribbon ramp
    float pulse = 0.0f;                 // audio level, fast attack / Decay release
    float energy = 0.0f;                // note envelope, eased
    float life = 0.0f;                  // 0 idle → 1 playing
    float fracture = 0.0f;              // eased fracture activity (0 when off)
    float freezeMix = 0.0f;             // 0 flowing → 1 frozen
    int   fps = 30;
    int   frameCounter = 0;

    // Adaptive quality / profiling
    int   quality = 0;
    // Per-frame budget: the quality level capped by the porthole's size, computed
    // once in integrate() so the painter draws exactly what was advanced.
    int   capRibbons = 22, capSamples = 44, capSparkles = 130, capBubbles = 18;
    bool  capShadows = true, capBloom = true, capSmudges = true;
    int   sparklesAlive = 0, bubblesAlive = 0;
    float paintMsAverage = 0.0f;
    int   framesSinceQualityChange = 0;
    int   recoveryWaitFrames = 150;     // doubles after every drop so a marginal machine does not oscillate
    bool  profileToStderr = false;
    int   pinnedQuality = -1;           // ANTIMATR_VIS_QUALITY pins the level for measurement
    static constexpr int kLayers = 11;
    std::array<double, kLayers> frameLayerMs {}, layerMsAverage {};
    juce::int64 lastTick = 0;

    // Procedural sources
    ValueNoise     noise;
    Iridescence    iridescence;
    LiquidOrganism organism;
    ParticleSystem particles;

    // Preallocated geometry: one frame of ribbons, their projection and their spans.
    std::array<std::array<RibbonSample, LiquidOrganism::kMaxSamples>, LiquidOrganism::kMaxRibbons> samples {};
    std::array<std::array<Projected, LiquidOrganism::kMaxSamples>, LiquidOrganism::kMaxRibbons> screen {};
    std::array<int, LiquidOrganism::kMaxRibbons> sampleLength {};
    std::array<RibbonSpan, LiquidOrganism::kMaxSpans> spans {};
    int numRibbons = 0, numSpans = 0, firstFrontSpan = 0;

    juce::Path ribbonPath, corePath, scratchPath, glassPath;
    /** Furthest any part of the object reaches from the centre, in pixels, this frame. */
    float objectExtent = 0.0f;
    juce::ColourGradient gradient;      // reused so a per-segment fill never allocates

    // Cached hardware: rendered at device resolution, redrawn only when the size changes.
    juce::Image wellImage, bezelImage, glassImage, plinthImage;
    juce::Rectangle<int> wellArea, bezelArea, glassArea, plinthArea;   // logical pixels
    void blit (juce::Graphics& g, const juce::Image& image, juce::Rectangle<int> area) const;
    float hardwareScale = 0.0f;
    int   hardwareWidth = 0, hardwareHeight = 0, hardwareSpace = -1;
};

} // namespace am::ui
