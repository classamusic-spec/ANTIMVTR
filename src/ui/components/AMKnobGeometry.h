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
    The smallest a knob can be and still carry readable dots and a cap.

    Deliberately clear of the sizes the instrument actually lays out (the small
    strips sit near 26 px, the ordinary clusters from the mid fifties up), so a
    knob never flips between two bodies as a window is resized.
*/
inline constexpr float kCappedMinDiameter = 46.0f;

/**
    How many LEDs fit around a knob of this size.

    Always odd, so one dot lands exactly in the middle of the sweep and a
    bipolar control has a true centre to light out from.
*/
inline int dotCount (float diameter) noexcept
{
    int n = juce::jlimit (9, 29, (int) std::lround (diameter * 0.21f));
    if ((n % 2) == 0) ++n;
    return n;
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

    // Clamped rather than floored: a knob far too small to wear this dress collapses
    // to nothing instead of growing a body larger than the ring around it.
    const float gap = juce::jmax (1.8f, diameter * 0.042f);
    const float capped = juce::jlimit (0.0f, r.ledRadius, r.ledRadius - r.dotRadius - gap);

    if (style == Style::Plain)
    {
        // No ring to clear, so the dome takes the room the dots would have used.
        // The knob still fills the footprint it was given — it is the body that is
        // larger, which is exactly what makes the plain knob read as the quiet one.
        r.bodyRadius = juce::jmin (r.modRadius, r.ledRadius + r.dotRadius * 0.30f);
    }
    else
    {
        r.bodyRadius = capped;
        r.capRadius = capped * 0.72f;
        r.dots = dotCount (diameter);
    }
    return r;
}

} // namespace am::ui::knobart
