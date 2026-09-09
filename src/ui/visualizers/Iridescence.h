#pragma once

#include <array>
#include <cmath>
#include "ui/AntiMatrTheme.h"

namespace am::ui
{

/**
    Thin-film / Fresnel colour helpers for the ANTI-MATTER OBJECT.

    The iridescent cycle runs blue → violet → magenta → cyan → ivory and
    wraps. A 256-entry lookup table is computed once so per-frame colour
    picks are a table read, never an allocation.
*/
class Iridescence
{
public:
    static constexpr int kTableSize = 256;

    Iridescence()
    {
        struct Stop { float position; juce::Colour colour; };
        const Stop stops[] = {
            { 0.00f, Theme::blue },
            { 0.20f, Theme::violet },
            { 0.42f, Theme::magenta },
            { 0.64f, Theme::cyan },
            { 0.84f, Theme::ivory },
            { 1.00f, Theme::blue },
        };
        constexpr int numStops = (int) (sizeof (stops) / sizeof (stops[0]));
        for (int i = 0; i < kTableSize; ++i)
        {
            const float u = (float) i / (float) kTableSize;
            int s = 0;
            while (s < numStops - 2 && u >= stops[s + 1].position) ++s;
            const float span = stops[s + 1].position - stops[s].position;
            const float t = span > 0.0f ? (u - stops[s].position) / span : 0.0f;
            table[(size_t) i] = stops[s].colour.interpolatedWith (stops[s + 1].colour, smooth (t));
        }
    }

    /** Colour at cyclic position u (any float; wraps). */
    juce::Colour at (float u) const noexcept
    {
        u -= std::floor (u);
        return table[(size_t) juce::jlimit (0, kTableSize - 1, (int) (u * (float) kTableSize))];
    }

    /** Colour at u, pulled toward ivory-white by `whiten` (0..1) — for lit rims and highlights. */
    juce::Colour lit (float u, float whiten) const noexcept
    {
        return at (u).interpolatedWith (Theme::ivory, juce::jlimit (0.0f, 1.0f, whiten));
    }

    /**
        Fresnel-style rim weight for a unit normal: bright where the surface
        faces the key light (top-left) with a softer rim light from the
        opposite side, dim in between. Returns roughly 0.15 .. 1.25.
    */
    static float fresnel (float nx, float ny) noexcept
    {
        constexpr float k1x = -0.60f, k1y = -0.80f;    // key light, top-left
        constexpr float k2x =  0.72f, k2y =  0.55f;    // rim light, bottom-right
        const float d1 = juce::jmax (0.0f, nx * k1x + ny * k1y);
        const float d2 = juce::jmax (0.0f, nx * k2x + ny * k2y);
        return 0.15f + 0.85f * d1 * std::sqrt (d1) + 0.45f * d2 * d2;
    }

private:
    static float smooth (float t) noexcept { return t * t * (3.0f - 2.0f * t); }

    std::array<juce::Colour, kTableSize> table;
};

} // namespace am::ui
