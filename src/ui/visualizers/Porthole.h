#pragma once

#include <cmath>
#include "LiquidOrganism.h"

namespace am::ui
{

/**
    Geometry of the glass porthole (VISUAL_SPEC §5), derived entirely from the
    component's bounds so the instrument is identical in proportion at 1100×690
    and at 1600×1000.

    Kept in plain floats (no juce::Rectangle) so the layout can be unit tested in
    the console test runner, which links no graphics module.

        ┌──────────────────────────────────────┐
        │ INHALE                       EXHALE  │  caption band
        │ IDEA                         EVOLVE  │
        │            ╭──────────╮              │  bezel  (outerR)
        │           │  ╭──────╮  │             │  glass  (glassR)
        │        ▌  │  │object│  │  ▌          │  status lamps at 9 and 3
        │           │  ╰──────╯  │             │
        │            ╰──────────╯              │
        │          ▁▁▁▁▁▁▁▁▁▁▁▁▁▁              │  plinth (ellipse)
        └──────────────────────────────────────┘
*/
struct PortholeLayout
{
    float centreX = 0.0f, centreY = 0.0f;   ///< centre of the porthole
    float outerR = 1.0f;                    ///< outer edge of the metal bezel
    float bezelWidth = 1.0f;                ///< radial thickness of the bezel
    float glassR = 1.0f;                    ///< inner radius: the glass
    float captionBand = 0.0f;               ///< height reserved above the bezel for the flank labels
    float plinthTop = 0.0f;                 ///< y of the top of the plinth ellipse
    float plinthHalfWidth = 1.0f;
    float plinthHeight = 1.0f;
    float unit = 1.0f;                      ///< one reference pixel: outerR / 186 (the design radius)

    /** Layout for a component of `w` × `h` logical pixels with its top-left at (x, y). */
    static PortholeLayout forBounds (float x, float y, float w, float h) noexcept
    {
        PortholeLayout l;
        if (! (std::isfinite (x) && std::isfinite (y) && std::isfinite (w) && std::isfinite (h)))
            return l;

        w = liquid::clampf (w, 1.0f, 20000.0f);
        h = liquid::clampf (h, 1.0f, 20000.0f);

        // The caption band and the plinth claim fixed fractions of the height; the
        // porthole takes whatever is left, capped so it never touches the sides.
        l.captionBand = h * 0.085f;
        const float plinthBand = h * 0.155f;
        const float available = h - l.captionBand - plinthBand * 0.62f;
        const float diameter = liquid::clampf (w * 0.80f < available ? w * 0.80f : available, 8.0f, 20000.0f);

        l.outerR = diameter * 0.5f;
        l.centreX = x + w * 0.5f;
        l.centreY = y + l.captionBand + l.outerR;
        l.bezelWidth = l.outerR * 0.093f;
        l.glassR = l.outerR - l.bezelWidth;
        l.unit = l.outerR / 186.0f;

        l.plinthTop = l.centreY + l.outerR * 0.90f;
        l.plinthHalfWidth = liquid::clampf (l.outerR * 1.06f, 4.0f, w * 0.48f);
        l.plinthHeight = liquid::clampf (y + h - l.plinthTop, 4.0f, plinthBand * 1.4f);
        return l;
    }

    /** True when the component is too small to be worth drawing the hardware into. */
    bool isTiny() const noexcept { return outerR < 24.0f; }
};

} // namespace am::ui
