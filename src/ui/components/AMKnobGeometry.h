#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

/**
    The arithmetic behind a knob, and nothing else.

    Concentric radii, dot counts and which of the three bodies a size can carry —
    no components, no graphics, no colour — so the numbers the knobs are built
    from can be reasoned about and unit tested without a screen. AMKnobArt.h
    turns these into rectangles and paint; it never invents a size of its own.
*/
namespace am::ui::knobart
{

/** Which of the three bodies a knob wears (SPEC section 2). */
enum class Style
{
    Auto,        ///< chosen from the knob's role and size (see AMKnob::effectiveStyle)
    CappedLit,   ///< A — metal cap, LED ring lit to the value
    CappedDark,  ///< B — metal cap, every dot dark
    Plain        ///< C — matte dome, no cap and no dots
};

/**
    The sweep every knob turns through: 7 o'clock to 5 o'clock, as the reference
    shows, which leaves the bottom of the ring open for the source-count badge.
*/
inline constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.25f;
inline constexpr float kEndAngle   = juce::MathConstants<float>::pi * 2.75f;

/**
    How many LEDs the ring carries.

    A fixed number, because the ring is one piece of hardware drawn larger or
    smaller — not a ring that grows lamps as it grows. Everything else about a
    knob scales with its diameter, so a constant count keeps the gaps between
    dots in exactly the same proportion at every size. Odd, so one dot lands in
    the middle of the sweep and a bipolar control has a true centre to light
    out from.
*/
inline constexpr int kDotCount = 25;

/**
    Below this diameter the ring is dropped.

    A dot cannot shrink past about a pixel and still be a dot, so under this size
    the ring stops being a readout and becomes a smear around the knob. Rather
    than draw it badly, the knob sheds it and spends the room on the body — the
    cap and the indicator survive at every size, which is what keeps a 20 px knob
    reading as a small knob instead of a broken one.
*/
inline constexpr float kRingMinDiameter = 44.0f;

/** How many LEDs a knob of this size carries; 0 when it is too small for a ring. */
inline int dotCount (float diameter) noexcept
{
    return diameter >= kRingMinDiameter ? kDotCount : 0;
}

/**
    The concentric radii of one knob, from the rim inwards.

    The modulation orbit is reserved whether or not the knob is modulated, so
    adding a routing never resizes anything; the LED ring sits inside it with
    air on both sides, and the body is a clear gap further in so the ring always
    reads as separate hardware from the knob it surrounds.
*/
struct Radii
{
    float modStroke = 1.0f;   ///< stroke of the modulation arc
    float modRadius = 0.0f;   ///< radius of the modulation arc (outermost)
    float ledRadius = 0.0f;   ///< the circle the LED dots sit on
    float dotRadius = 0.0f;   ///< radius of one dot
    float bodyRadius = 0.0f;  ///< the moulded body / matte dome
    float capRadius = 0.0f;   ///< the turned metal cap (0 when plain)
    int   dots = 0;           ///< 0 when the style carries no ring

    /** Clear space between the outside of a dot and the modulation arc. */
    float orbitGap() const noexcept { return modRadius - modStroke * 0.5f - (ledRadius + dotRadius); }
    /** Clear space between the inside of a dot and the edge of the body. */
    float bodyGap() const noexcept { return (ledRadius - dotRadius) - bodyRadius; }
};

inline Radii radii (float diameter, Style style) noexcept
{
    Radii r;
    if (diameter <= 0.0f) return r;

    r.modStroke = juce::jmax (1.0f, diameter * 0.012f);
    r.modRadius = juce::jmax (0.0f, diameter * 0.5f - r.modStroke * 0.8f);
    r.dotRadius = juce::jmax (1.1f, diameter * 0.0265f);
    r.ledRadius = juce::jmax (0.0f, r.modRadius - r.modStroke * 1.6f - r.dotRadius * 1.35f);

    // Two sizes for the body: the one that leaves a dark gap inside a ring of dots,
    // and the one that takes the ring's room when there is no ring to leave it for.
    // Clamped rather than floored, so a knob far too small for this dress collapses
    // to nothing instead of growing a body larger than the orbit around it.
    const float gap = juce::jmax (1.8f, diameter * 0.042f);
    const float ringed = juce::jlimit (0.0f, r.ledRadius, r.ledRadius - r.dotRadius - gap);
    const float filled = juce::jmin (r.modRadius, r.ledRadius + r.dotRadius * 0.30f);

    if (style == Style::Plain)
    {
        // No ring to clear, so the dome takes the room the dots would have used.
        // The knob still fills the footprint it was given — it is the body that is
        // larger, which is exactly what makes the plain knob read as the quiet one.
        r.bodyRadius = filled;
    }
    else
    {
        r.dots = dotCount (diameter);
        r.bodyRadius = r.dots > 0 ? ringed : filled;
        r.capRadius = r.bodyRadius * 0.72f;
    }
    return r;
}

} // namespace am::ui::knobart
