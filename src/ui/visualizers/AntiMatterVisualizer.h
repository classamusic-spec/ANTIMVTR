#pragma once

#include "ui/components/AMDrawing.h"
#include "dev/diagnostics/Diagnostics.h"
#include "ValueNoise.h"
#include "Iridescence.h"
#include "LiquidOrganism.h"
#include "NodeField.h"
#include "FieldRenderer.h"
#include "Porthole.h"

namespace am::ui
{

/**
    THE CENTRE — a glass porthole with the ANTI-MATTER object alive behind it.

    This is the identity of the product (VISUAL_SPEC §5). Everything is drawn
    procedurally from `getLocalBounds()`: no bitmaps, no assets, nothing loaded
    from disk.

    THE OBJECT IS A VOLUMETRIC FIELD OF LIGHT — thousands of points filling a
    sphere, painted with real depth, and every one of them bound to a resonator
    of the physical model that is making the sound (see NodeField). A point
    behind the core is small, dim, cool and soft; one at the front is large,
    bright, saturated and sharp. The whole mass drifts and folds through a slow
    3D flow field rather than spinning rigidly, so it reads as something you
    could walk around rather than a picture printed on the glass.

    Layers, back to front:

      2. The glass well: a shallow bowl whose colours come from Theme, tinted by
         the Space type. No chassis colour is baked in here.
      3. The dark core — the object's own mass — with the violet rim where the
         light wraps around it. Points behind it are masked out against its
         silhouette, which is what lets the whole field be accumulated in one
         additive pass instead of two.
      4. THE FIELD, clipped to the glass: every point splatted additively into a
         float buffer (FieldRenderer), tone mapped once and blitted. Overlapping
         points sum and bloom toward white instead of flat-shading over one
         another, and a fast point leaves a short comet tail.
      5. The glass itself: a broad diagonal specular sweep, a crescent under the
         top of the bezel, thickness darkening to the lower right, a chromatic
         fringe at the extreme edge and slow drifting smudges.
      6. The bezel: brushed gunmetal segmented into plates with seams and bolt
         heads, an outer shadow onto the panel and an inner shadow onto the glass.
      7. The two warm status lamps at nine and three o'clock.
      8. The flank captions (INHALE / IDEA, EXHALE / EVOLVE).

    The hardware (well, bezel, glass highlights) is static geometry, so it is
    rendered once into cached images and blitted; only the object and the lamp
    bloom are redrawn each frame. Paint time is measured every frame and the
    quality level drops when it runs long.

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

    /** How many points of light the field is carrying this frame. */
    int pointCount() const noexcept { return livePoints; }

private:
    /** Everything derived once per paint from the eased state and the component bounds. */
    struct Frame
    {
        juce::Rectangle<float> bounds;
        PortholeLayout port;
        juce::Point<float> centre;      ///< centre of the object (== centre of the porthole)
        float R = 1.0f;                 ///< pixel radius of the implicit sphere
        float unit = 1.0f;              ///< one reference pixel (outerR / 170)
        float coreR = 0.3f;             ///< core radius, object units
        float breathe = 1.0f;
        juce::Colour space;

        // this frame's budget (quality level capped by the porthole's size)
        int   maxPoints = 2400;
        float bufferScale = 1.0f;       ///< accumulation buffer resolution vs. the glass
        bool  trails = true, wideHalo = true, smudges = true;

        // the object's rotation (yaw then a fixed pitch), precomputed
        float cosYaw = 1.0f, sinYaw = 0.0f, cosPitch = 1.0f, sinPitch = 0.0f;

        float pulse = 0.0f, energy = 0.0f, life = 0.0f, fracture = 0.0f, hue = 0.0f;
        float spaceActivity = 0.0f, freezeMix = 0.0f, crush = 0.0f;
        float shock = 0.0f, strike = 0.0f;
    };

    void timerCallback() override;
    void integrate (float dt);
    void updateQuality();
    void mark (int layer) noexcept;
    void computeBudget();

    // ---- the object
    void drawWell (juce::Graphics& g, const Frame& f);
    void renderWell (juce::Graphics& g, const Frame& f);
    void buildCoreOutline (const Frame& f);
    void drawCore (juce::Graphics& g, const Frame& f);
    void drawField (juce::Graphics& g, const Frame& f);

    // ---- the hardware (Porthole.cpp)
    void refreshHardware (const Frame& f, float deviceScale);
    void renderBezel (juce::Graphics& g, const Frame& f);
    void renderGlass (juce::Graphics& g, const Frame& f);
    void drawLamps (juce::Graphics& g, const Frame& f);
    void drawSmudges (juce::Graphics& g, const Frame& f);
    void drawCaptions (juce::Graphics& g, const Frame& f);

    Diagnostics& diag;
    VisualStateSnapshot latest;
    VisualStateSnapshot smooth;         // eased values for inertia

    // Animation state
    float time = 0.0f;                  // wall clock
    float flowTime = 0.0f;              // flow-field clock — held still by Freeze
    float rotation = 0.0f;              // yaw of the field
    float hueDrift = 0.0f;              // slow travel along the ramp
    float pulse = 0.0f;                 // audio level, fast attack / Decay release
    float energy = 0.0f;                // note envelope, eased
    float life = 0.0f;                  // 0 idle → 1 playing
    float fracture = 0.0f;              // eased fracture activity (0 when off)
    float freezeMix = 0.0f;             // 0 flowing → 1 frozen
    int   fps = 30;
    int   frameCounter = 0;

    // Adaptive quality / profiling
    int   quality = 0;
    int   capPoints = 2400;
    float capBuffer = 1.0f;
    bool  capTrails = true, capWideHalo = true, capSmudges = true;
    int   livePoints = 0;
    float paintMsAverage = 0.0f;
    int   framesSinceQualityChange = 0;
    int   recoveryWaitFrames = 150;     // doubles after every drop so a marginal machine does not oscillate
    bool  profileToStderr = false;
    int   pinnedQuality = -1;           // ANTIMATR_VIS_QUALITY pins the level for measurement
    static constexpr int kLayers = 8;
    std::array<double, kLayers> frameLayerMs {}, layerMsAverage {};
    juce::int64 lastTick = 0;

    // Procedural sources
    ValueNoise     noise;
    Iridescence    iridescence;
    NodeField      field;
    FieldRenderer  renderer;

    /** The object's colour, precomputed: hue around the ramp × depth into the volume. */
    static constexpr int kHueBins = 96, kDepthBins = 12;
    std::array<float, (size_t) (kHueBins * kDepthBins * 3)> shade {};
    void buildShadeTable();

    /** The core silhouette, in pixels, sampled around the circle. Shared by the
        painter and the depth mask so a point can never show through the mass. */
    static constexpr int kCoreSegs = 72;
    std::array<float, (size_t) kCoreSegs> coreOutline {};
    float coreOutlineMax = 0.0f;

    juce::Path corePath, glassPath;
    juce::ColourGradient gradient;      // reused so a per-segment fill never allocates

    // Cached hardware: rendered at device resolution, redrawn only when the size changes.
    juce::Image wellImage, bezelImage, glassImage;
    juce::Rectangle<int> wellArea, bezelArea, glassArea;   // logical pixels
    void blit (juce::Graphics& g, const juce::Image& image, juce::Rectangle<int> area) const;
    float hardwareScale = 0.0f;
    int   hardwareWidth = 0, hardwareHeight = 0, hardwareSpace = -1;
};

} // namespace am::ui
