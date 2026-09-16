#pragma once

#include <juce_core/juce_core.h>

/**
    Pure layout arithmetic for the interface.

    Everything here is a function of a component's own bounds and nothing
    else — no components, no graphics, no state — so the numbers behind the
    look can be reasoned about and unit tested without a screen. The drawing
    code turns these into rectangles; it never invents sizes of its own.
*/
namespace am::ui::layout
{

/** A horizontal slice of a row. */
struct Span
{
    int start = 0, size = 0;
    int end() const noexcept { return start + size; }
    bool isEmpty() const noexcept { return size <= 0; }
};

//==============================================================================
/** Column geometry of a modulation routing row, shared by the rows and the table header. */
struct ModRowColumns
{
    Span chip, source, arrow, destination, depth, value, power, polarity, remove;
    int chipInsetY = 0;        ///< vertical inset of the destination-colour chip
    int polarityHeight = 0;    ///< the BI / UNI switch is shorter than the row
    int gap = 0;
};

inline ModRowColumns modRowColumns (int width, int height) noexcept
{
    ModRowColumns c;
    const int h = juce::jmax (1, height);
    const int w0 = juce::jmax (0, width);
    c.gap = juce::jmax (4, h / 5);

    const int chipW = juce::jmin (juce::jmax (2, h / 10), w0);
    c.chip = { 0, chipW };
    c.chipInsetY = juce::jmax (1, h / 6);

    const int pad = juce::jmax (7, h / 3);
    const int left = juce::jmin (chipW + pad, w0);
    int right = juce::jmax (left, w0 - pad);
    const int inner = juce::jmax (1, right - left);

    // Everything on the right is taken in order and clamped into what is left, so a row that
    // is far too narrow degenerates into empty columns instead of drawing off its own edge.
    auto takeRight = [&right, left] (int want) -> Span
    {
        const int size = juce::jlimit (0, juce::jmax (0, right - left), want);
        right -= size;
        return { right, size };
    };
    auto skipRight = [&right, left] (int want) { right = juce::jmax (left, right - juce::jmax (0, want)); };

    c.remove = takeRight (juce::jlimit (18, 30, h));
    skipRight (c.gap);
    c.polarity = takeRight (juce::jlimit (58, 88, inner / 12));
    c.polarityHeight = juce::jlimit (17, 26, h * 2 / 3);
    skipRight (c.gap);
    c.power = takeRight (juce::jlimit (28, 40, inner / 24));
    skipRight (c.gap * 2);
    c.value = takeRight (juce::jlimit (50, 78, inner / 12));
    skipRight (c.gap);

    const int names = juce::jlimit (140, 420, (int) ((float) inner * 0.34f));
    const int blockW = juce::jlimit (0, juce::jmax (0, right - left), juce::jmin (names, juce::jmax (60, (right - left) - 60)));
    const int sourceW = juce::jmin (blockW, juce::jmax (44, (int) ((float) blockW * 0.36f)));
    const int arrowW = juce::jmin (blockW - sourceW, juce::jmax (12, h / 2));
    c.source      = { left, sourceW };
    c.arrow       = { left + sourceW, arrowW };
    c.destination = { left + sourceW + arrowW, juce::jmax (0, blockW - sourceW - arrowW) };

    const int depthStart = juce::jmin (left + blockW + c.gap, right);
    c.depth = { depthStart, juce::jmax (0, right - depthStart) };
    return c;
}

//==============================================================================
/**
    The three columns every deep page is built from: a selector column on the
    left, the large dark display in the middle and the control cluster on the
    right.

    Pure fractions of the page, with no pixel constants anywhere, so the same
    composition holds at 1100 x 690 and at 1600 x 1000 — the display always keeps
    the middle and the flanks always keep their share of the width.
*/
struct PageColumns
{
    Span sidebar, centre, controls;
};

inline PageColumns pageColumns (int width, int gap, float sidebarFraction = 0.165f, float controlsFraction = 0.295f) noexcept
{
    PageColumns c;
    const int w = juce::jmax (0, width);
    const int usable = juce::jmax (0, w - juce::jmax (0, gap) * 2);
    const int side  = juce::roundToInt ((float) usable * juce::jlimit (0.0f, 0.5f, sidebarFraction));
    const int right = juce::roundToInt ((float) usable * juce::jlimit (0.0f, 0.5f, controlsFraction));
    c.sidebar  = { 0, side };
    c.centre   = { side + gap, juce::jmax (0, usable - side - right) };
    c.controls = { w - right, right };
    return c;
}

//==============================================================================
/**
    Number of columns for the source-scope grid: as square as the count allows,
    never so many that a card falls below a readable width, and preferring a
    full last row over a ragged one.
*/
inline int modScopeColumns (int count, int width) noexcept
{
    if (count <= 0) return 1;
    const int maxByWidth = juce::jlimit (1, 4, width / 190);
    int cols = count <= 4 ? count : (count <= 6 ? 3 : 4);
    cols = juce::jmin (cols, maxByWidth);
    while (cols > 2 && count % cols != 0 && count % (cols - 1) == 0) --cols;
    return juce::jmax (1, cols);
}

//==============================================================================
/**
    Column count for a grid of equal controls in an area of `width` x `height`.

    Chooses the arrangement that leaves each control the largest it can be —
    a knob needs room for its label as well as its circle — and, between
    arrangements that are effectively the same size, the one that leaves the
    fewest empty cells. Choosing from the width alone gave a six-knob module
    two columns and three rows of 26px knobs while the panel next door, with
    one knob, drew it at 90px.
*/
inline int gridColumns (int count, int width, int height) noexcept
{
    if (count <= 1 || width <= 0 || height <= 0) return juce::jmax (1, count);

    int best = 1;
    float bestSize = -1.0f;
    int bestEmpty = count;
    for (int cols = 1; cols <= count; ++cols)
    {
        const int rows = (count + cols - 1) / cols;
        const float cellW = (float) width / (float) cols;
        const float cellH = (float) height / (float) rows;
        const float labelBand = juce::jlimit (10.0f, 22.0f, cellH * 0.19f) + 2.0f;
        float size = juce::jmin (cellW, cellH - labelBand);
        // A cell narrower than its caption is a false economy: the knob is big and the
        // label under it is clipped, so squeezed columns are scored down.
        constexpr float readableLabel = 68.0f;
        if (cellW < readableLabel) size -= (readableLabel - cellW) * 0.6f;
        const int empty = rows * cols - count;

        if (size > bestSize + 2.0f || (size > bestSize - 2.0f && empty < bestEmpty))
        {
            if (size > bestSize) bestSize = size;
            bestEmpty = empty;
            best = cols;
        }
    }
    return best;
}

//==============================================================================
/**
    The margin a panel keeps inside its own bounds so its drop shadow has
    somewhere to fall. Nothing may paint outside a component, so the slab is
    drawn this far in and the shadow lands in the margin.

    On a pale chassis that shadow is the only thing holding a panel off the
    ground, so this is generous: too small a margin clips the soft edge of the
    shadow and the panel flattens back into the background.
*/
inline float panelShadowMargin (float width, float height) noexcept
{
    if (width <= 0.0f || height <= 0.0f) return 0.0f;
    return juce::jlimit (4.0f, 11.0f, juce::jmin (width, height) * 0.022f);
}

/** Inner padding of a panel: the gutter its title and content sit in. */
inline float panelPadding (float width) noexcept
{
    return juce::jlimit (8.0f, 22.0f, width * 0.042f);
}

/**
    The four screws that bolt a panel to the chassis: where their centres sit and
    how big their heads are. `headerLeft` is the first x a title may use without
    touching the top-left screw, and it is always inside the panel's padding.
*/
struct PanelHardware
{
    float inset = 0.0f;    ///< distance from each edge to a screw centre
    float radius = 0.0f;   ///< screw head radius (0 when the panel is too small to bolt down)

    float headerLeft() const noexcept { return inset + radius; }
    bool  isEmpty() const noexcept { return radius <= 0.0f; }
};

inline PanelHardware panelHardware (float width, float height) noexcept
{
    PanelHardware h;
    const float small = juce::jmin (width, height);
    if (small < 26.0f) return h;
    h.radius = juce::jlimit (2.0f, 4.0f, small * 0.0085f);
    h.inset  = juce::jlimit (6.0f, 13.0f, small * 0.024f);
    return h;
}

//==============================================================================
/**
    How a horizontal slider divides its row: a label column, the capsule track
    and a right-aligned value column, plus the sizes of the track and its handle.

    The track is inset by the handle's radius at both ends so the handle never
    leaves the row, whatever the value.
*/
struct SliderRow
{
    float labelWidth = 0.0f, valueWidth = 0.0f;
    float trackX = 0.0f, trackWidth = 0.0f;
    float trackHeight = 0.0f, handleRadius = 0.0f;

    float labelX() const noexcept { return 0.0f; }
    float valueX() const noexcept { return trackX + trackWidth + handleRadius + 3.0f; }
    /** Centre of the handle for a normalised position. */
    float handleX (float proportion) const noexcept { return trackX + trackWidth * juce::jlimit (0.0f, 1.0f, proportion); }
};

inline SliderRow sliderRow (float width, float height, bool withLabel, bool withValue) noexcept
{
    SliderRow r;
    if (width <= 0.0f || height <= 0.0f) return r;
    r.labelWidth = withLabel ? juce::jmin (width * 0.24f, 92.0f) : 0.0f;
    r.valueWidth = withValue ? juce::jmin (width * 0.16f, 58.0f) : 0.0f;
    r.handleRadius = juce::jlimit (4.5f, 9.0f, height * 0.24f);
    r.trackHeight = juce::jlimit (4.0f, 11.0f, r.handleRadius * 1.15f);

    const float middle = juce::jmax (0.0f, width - r.labelWidth - r.valueWidth);
    const float margin = r.handleRadius + 3.0f;
    // A row too narrow for the handle keeps a zero-length track rather than a
    // negative one: the handle then sits still instead of drawing outside the row.
    r.trackWidth = juce::jmax (0.0f, middle - margin * 2.0f);
    r.trackX = r.labelWidth + juce::jmin (margin, middle * 0.5f);
    return r;
}

//==============================================================================
/**
    Concentric radii of a knob, from the rim inwards: the modulation orbit, a
    clear moat, the value arc and the sphere body. All values are diameters in
    the same units as `diameter`, which is the knob's square footprint.
*/
struct KnobRadii
{
    float ringStroke = 1.1f, trackWidth = 1.4f;
    float orbit = 0.0f, arc = 0.0f, body = 0.0f;

    /** Clear space between the outer edge of the value arc and the middle of the orbit. */
    float moat() const noexcept { return (orbit - arc) * 0.5f - trackWidth * 0.5f; }
};

/**
    Vertical placement of a grid of equal controls.

    A control is a round body with a caption under it, so it can never use more
    height than a little over its own width. A cell taller than that stretches
    the rows apart and leaves a dead band across the middle of the panel, so the
    rows are capped and the block is centred: the slack becomes an even border
    instead of a hole.
*/
struct GridRows
{
    int cellHeight = 0;
    int top = 0;        ///< y of the first row, relative to the area's top
    int pitch = 0;      ///< top-to-top distance between rows (cellHeight + gap, plus any share of the slack)
};

/**
    `spread` shares the slack out between the rows instead of leaving it all as a
    border. A control can never use more height than a little over its own width,
    so in a panel much taller than its content the centred block leaves a dead
    band above it and another below; spreading turns those two bands into even
    air between the rows, which is what a tall cluster panel wants.
*/
inline GridRows gridRows (int areaHeight, int rows, int cellWidth, int gapY, bool spread = false) noexcept
{
    GridRows g;
    if (rows <= 0 || areaHeight <= 0) return g;
    const int even    = juce::jmax (1, (areaHeight - gapY * (rows - 1)) / rows);
    const int natural = juce::jmax (1, juce::roundToInt ((float) juce::jmax (0, cellWidth) * 1.22f) + 12);
    g.cellHeight = juce::jmin (even, natural);
    g.pitch = g.cellHeight + gapY;

    const int block = g.cellHeight * rows + gapY * (rows - 1);
    if (spread && rows > 1 && areaHeight > block)
    {
        // The slack is shared between every gap there is — one above the first row,
        // one between each pair, one below the last — so the air is even everywhere
        // rather than pooling in the middle.
        const int share = (areaHeight - block) / (rows + 1);
        g.pitch += share;
        g.top = juce::jmax (0, (areaHeight - (g.pitch * (rows - 1) + g.cellHeight)) / 2);
        return g;
    }
    g.top = juce::jmax (0, (areaHeight - block) / 2);
    return g;
}

/**
    The largest a knob may grow, whatever room its cell has. A hero knob is the
    subject of its panel and is allowed to be much bigger than an ordinary one;
    without a ceiling a lone control in a tall panel would swell to fill it, and
    with too low a ceiling the panel is left with a dead band across its middle.
*/
inline float knobDiameterCap (bool hero) noexcept { return hero ? 200.0f : 118.0f; }

/** How a knob's circle and its caption share the cell they are given. */
struct KnobFootprint
{
    float diameter = 0.0f;
    float top = 0.0f;          ///< y of the circle, relative to the cell's top
    float labelHeight = 0.0f;  ///< the caption band under the circle (0 without one)

    /** Total height of the circle and its caption together. */
    float groupHeight() const noexcept { return diameter + (labelHeight > 0.0f ? labelHeight + 2.0f : 0.0f); }
};

inline KnobFootprint knobFootprint (float width, float height, bool hero, bool withLabel) noexcept
{
    KnobFootprint f;
    if (width <= 0.0f || height <= 0.0f) return f;
    f.labelHeight = withLabel ? juce::jlimit (10.0f, 22.0f, height * 0.19f) : 0.0f;
    // The width keeps a little more margin than the height: side by side in a row,
    // two knobs that each took their whole cell would have their arcs touching.
    f.diameter = juce::jmax (0.0f, juce::jmin (width * 0.94f, height - f.labelHeight - 2.0f, knobDiameterCap (hero)) * 0.99f);
    f.top = juce::jmax (0.0f, (height - f.groupHeight()) * 0.5f);
    return f;
}

inline KnobRadii knobRadii (float diameter, bool hero) noexcept
{
    KnobRadii r;
    if (diameter <= 0.0f) return r;
    r.ringStroke = juce::jmax (1.1f, diameter * 0.016f);
    r.trackWidth = juce::jmax (1.4f, diameter * (hero ? 0.030f : 0.026f));
    r.orbit = juce::jmax (0.0f, diameter - r.ringStroke * 2.8f);
    r.arc   = juce::jmax (0.0f, r.orbit - juce::jmin (r.ringStroke * 6.0f, r.orbit * 0.32f));
    r.body  = juce::jmax (0.0f, r.arc - juce::jmin (r.trackWidth * 3.4f, r.arc * 0.36f));
    return r;
}

//==============================================================================
/**
    Height at which a row of source thumbnails fills its cells. A thumbnail is
    0.76 of its cell and the label band is 20% of the height, so the circle
    stops growing once the height passes about 1.28 cells; anything taller is
    empty space.
*/
inline int sourceSelectorHeight (int width, int items) noexcept
{
    if (items <= 0 || width <= 0) return 0;
    const float cell = (float) width / (float) items;
    return juce::roundToInt (juce::jlimit (52.0f, 170.0f, cell) * 1.28f);
}

//==============================================================================
/** Tick spacing (in seconds) for a waveform ruler: keeps roughly 4 to 9 divisions at any length. */
inline double waveRulerStep (double seconds) noexcept
{
    static const double steps[] = { 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0 };
    if (! (seconds > 0.0)) return steps[0];
    for (double candidate : steps)
        if (seconds / candidate <= 9.0) return candidate;
    return steps[std::size (steps) - 1];
}

//==============================================================================
/**
    How an envelope display shares its width between attack, decay, the sustain
    plateau and release.

    The three timed stages are shown in proportion to their lengths (compressed
    by a cube root so a 10 s release does not squeeze a 5 ms attack out of
    existence) and the plateau keeps a fixed share, so the same settings always
    draw the same shape and a longer stage always looks longer.
*/
struct EnvelopeStages
{
    float attack = 0.0f, decay = 0.0f, sustain = 0.0f, release = 0.0f;   ///< fractions of the width, summing to 1
};

inline EnvelopeStages envelopeStages (float attackSeconds, float decaySeconds, float releaseSeconds) noexcept
{
    auto weigh = [] (float seconds)
    {
        const float s = std::isfinite (seconds) ? juce::jmax (0.0f, seconds) : 0.0f;
        return std::cbrt (s + 0.002f);
    };
    constexpr float plateau = 0.22f;
    const float a = weigh (attackSeconds), d = weigh (decaySeconds), r = weigh (releaseSeconds);
    const float total = juce::jmax (1.0e-6f, a + d + r);
    const float scale = 1.0f - plateau;

    EnvelopeStages s;
    s.attack  = a / total * scale;
    s.decay   = d / total * scale;
    s.release = r / total * scale;
    s.sustain = plateau;
    return s;
}

//==============================================================================
/**
    A modulation depth written in the destination's own units: a signed share
    of the parameter's range, with the unit appended when it has one.
*/
inline juce::String modDepthText (float depth, float minValue, float maxValue, const char* unit)
{
    const float amount = depth * (maxValue - minValue);
    juce::String s = (amount >= 0.0f ? "+" : "") + juce::String (amount, std::abs (amount) < 10.0f ? 2 : 1);
    if (unit != nullptr && unit[0] != 0) s << " " << unit;
    return s;
}

} // namespace am::ui::layout
