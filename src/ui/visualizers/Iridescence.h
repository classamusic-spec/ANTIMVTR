#pragma once

#include <array>
#include <cmath>
#include "ui/AntiMatrTheme.h"
#include "LiquidOrganism.h"

namespace am::ui
{

/**
    Colour for the ANTI-MATTER object.

    The ribbon ramp (cyan → blue → violet → magenta → pink) is defined once, in
    plain floats, in `liquid::ramp` so it can be unit tested without a graphics
    module. This class bakes it into a 512-entry table of juce::Colour so a
    per-segment colour pick during paint is a table read and never an allocation,
    and adds the depth and light shading the painter needs on top.
*/
class Iridescence
{
public:
    static constexpr int kTableSize = 512;

    Iridescence()
    {
        for (int i = 0; i < kTableSize; ++i)
        {
            const auto c = liquid::ramp ((float) i / (float) kTableSize);
            table[(size_t) i] = toColour (c);
        }
    }

    static juce::Colour toColour (RGB c) noexcept
    {
        return juce::Colour::fromFloatRGBA (liquid::clampf (c.r, 0.0f, 1.0f),
                                            liquid::clampf (c.g, 0.0f, 1.0f),
                                            liquid::clampf (c.b, 0.0f, 1.0f), 1.0f);
    }

    /** Ramp colour at cyclic position u (any float; wraps). */
    juce::Colour at (float u) const noexcept
    {
        if (! std::isfinite (u)) u = 0.0f;
        u -= std::floor (u);
        const int i = (int) (u * (float) kTableSize);
        return table[(size_t) juce::jlimit (0, kTableSize - 1, i)];
    }

    /** Ramp colour pulled toward the molten white of a ribbon's core. */
    juce::Colour lit (float u, float whiten) const noexcept
    {
        return at (u).interpolatedWith (Theme::ivory, juce::jlimit (0.0f, 1.0f, whiten));
    }

    /**
        Depth shading. `depth` runs −1 (far side of the sphere) to +1 (nearest the
        glass). A ribbon behind the core loses saturation into the cold volume and
        goes dim; one in front keeps its colour and gains a little heat.
    */
    juce::Colour depthShade (float u, float depth, float whiten = 0.0f) const noexcept
    {
        depth = juce::jlimit (-1.0f, 1.0f, std::isfinite (depth) ? depth : 0.0f);
        auto c = liquid::ramp (u);
        if (depth < 0.0f) c = liquid::deepen (c, -depth * 0.62f);
        if (whiten > 0.0f) c = liquid::whiten (c, whiten);
        return toColour (c);
    }

    /**
        Fresnel-style rim weight for a unit normal: bright where the surface faces
        the key light (top-left) with a softer rim light from the opposite side,
        dim in between. Returns roughly 0.15 .. 1.25.
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
    std::array<juce::Colour, kTableSize> table;
};

} // namespace am::ui
