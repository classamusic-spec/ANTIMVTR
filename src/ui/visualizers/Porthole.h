#pragma once

#include <algorithm>
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
        └──────────────────────────────────────┘
*/
struct PortholeLayout
{
    float centreX = 0.0f, centreY = 0.0f;   ///< centre of the porthole
    float outerR = 1.0f;                    ///< outer edge of the metal bezel
    float bezelWidth = 1.0f;                ///< radial thickness of the bezel
    float glassR = 1.0f;                    ///< inner radius: the glass
    float captionBand = 0.0f;               ///< height reserved above the bezel for the flank labels
    float unit = 1.0f;                      ///< one reference pixel: outerR / 186 (the design radius)

    /** Layout for a component of `w` × `h` logical pixels with its top-left at (x, y). */
    static PortholeLayout forBounds (float x, float y, float w, float h) noexcept
    {
        PortholeLayout l;
        if (! (std::isfinite (x) && std::isfinite (y) && std::isfinite (w) && std::isfinite (h)))
            return l;

        w = liquid::clampf (w, 1.0f, 20000.0f);
        h = liquid::clampf (h, 1.0f, 20000.0f);

        // The porthole is the instrument's focal point, so it takes everything it can get:
        // it is centred in the component and sized by whichever of the two dimensions runs
        // out first. The flank captions sit in the top corners, where a centred circle
        // never reaches, so the ring is allowed to rise past their band.
        l.captionBand = h * 0.075f;
        const float margin = 0.985f;
        const float diameter = liquid::clampf (std::min (w, h) * margin, 8.0f, 20000.0f);

        l.outerR = diameter * 0.5f;
        l.centreX = x + w * 0.5f;
        l.centreY = y + h * 0.5f;
        l.bezelWidth = l.outerR * 0.125f;
        l.glassR = l.outerR - l.bezelWidth;
        l.unit = l.outerR / 170.0f;
        return l;
    }

    /** True when the component is too small to be worth drawing the hardware into. */
    bool isTiny() const noexcept { return outerR < 24.0f; }
};

} // namespace am::ui
