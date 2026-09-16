#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include "core/Random.h"
#include "ValueNoise.h"

namespace am::ui
{

/*
    THE OBJECT'S MATHS — the small, testable pieces every part of the centre
    visual is built out of, kept free of juce::Colour and juce::Rectangle so they
    can be exercised in the console test runner, which links no graphics module.

      Vec3 / RGB      the object's own space (the implicit sphere has radius 1)
                      and a plain linear colour triple
      liquid::ramp    the house colour ramp: cyan into blue into violet into
                      magenta into pink, wrapping back to cyan (VISUAL_SPEC §5)
      liquid::flow    the slowly evolving, near divergence-free 3D field the
                      whole volume advects through
      liquid::clean   NaN/infinity conditioning — a clamp passes a NaN straight
                      through, and one NaN would be a point drawn nowhere for
                      the rest of the run
*/

//==============================================================================
/** A point or direction in the object's own space (the implicit sphere has radius 1). */
struct Vec3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;

    constexpr Vec3 operator+ (const Vec3& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
    constexpr Vec3 operator- (const Vec3& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
    constexpr Vec3 operator* (float s) const noexcept       { return { x * s, y * s, z * s }; }
    Vec3& operator+= (const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator*= (float s) noexcept       { x *= s; y *= s; z *= s; return *this; }

    float lengthSquared() const noexcept { return x * x + y * y + z * z; }
    float length() const noexcept        { return std::sqrt (lengthSquared()); }
};

inline Vec3 cross (const Vec3& a, const Vec3& b) noexcept
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

inline float dot (const Vec3& a, const Vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 normalised (const Vec3& v, Vec3 fallback = { 1.0f, 0.0f, 0.0f }) noexcept
{
    const float l = v.length();
    return l > 1.0e-6f ? Vec3 { v.x / l, v.y / l, v.z / l } : fallback;
}

inline Vec3 mix (const Vec3& a, const Vec3& b, float t) noexcept
{
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

/** Linear RGB-ish triple in 0..1 — kept free of juce::Colour so the maths stays testable. */
struct RGB
{
    float r = 0.0f, g = 0.0f, b = 0.0f;
};

//==============================================================================
namespace liquid
{
    inline float clampf (float v, float lo, float hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

    /** NaN/inf-proof parameter conditioning: anything that is not finite becomes `fallback`. */
    inline float clean (float v, float lo = 0.0f, float hi = 1.0f, float fallback = 0.0f) noexcept
    {
        if (! std::isfinite (v)) return fallback;
        return clampf (v, lo, hi);
    }

    inline float smoothstep (float a, float b, float x) noexcept
    {
        if (! (b > a)) return x >= b ? 1.0f : 0.0f;
        const float t = clampf ((x - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

    /** Deterministic hash of an integer to [0, 1) — used for per-ribbon constants. */
    inline float hash01 (uint32_t i) noexcept
    {
        uint32_t x = i * 0x9E3779B9u;
        x = (x ^ (x >> 16)) * 0x7FEB352Du;
        x = (x ^ (x >> 15)) * 0x846CA68Bu;
        x ^= x >> 16;
        return (float) (x >> 8) * (1.0f / 16777216.0f);
    }

    /**
        The ribbon colour ramp (VISUAL_SPEC §5): cyan → blue → violet → magenta
        → pink, wrapping back to cyan so a ribbon may start anywhere in the range.
    */
    inline RGB ramp (float u) noexcept
    {
        if (! std::isfinite (u)) u = 0.0f;
        u -= std::floor (u);

        struct Stop { float at; RGB c; };
        static constexpr Stop stops[] = {
            { 0.00f, { 0.404f, 0.902f, 1.000f } },   // cyan     #67e6ff
            { 0.24f, { 0.310f, 0.553f, 1.000f } },   // blue     #4f8dff
            { 0.48f, { 0.561f, 0.388f, 1.000f } },   // violet   #8f63ff
            { 0.74f, { 0.894f, 0.333f, 0.812f } },   // magenta  #e455cf
            { 0.90f, { 1.000f, 0.561f, 0.769f } },   // pink     #ff8fc4
            { 1.00f, { 0.404f, 0.902f, 1.000f } },   // cyan again (wrap)
        };
        constexpr int numStops = (int) (sizeof (stops) / sizeof (stops[0]));

        int s = 0;
        while (s < numStops - 2 && u >= stops[s + 1].at) ++s;
        const float span = stops[s + 1].at - stops[s].at;
        const float raw = span > 1.0e-6f ? (u - stops[s].at) / span : 0.0f;
        const float t = clampf (raw, 0.0f, 1.0f);
        const float k = t * t * (3.0f - 2.0f * t);
        const RGB& a = stops[s].c;
        const RGB& b = stops[s + 1].c;
        return { lerp (a.r, b.r, k), lerp (a.g, b.g, k), lerp (a.b, b.b, k) };
    }

    /** Pulls a colour toward white (a molten core) or toward the cold grey of distance. */
    inline RGB toward (RGB c, RGB target, float t) noexcept
    {
        t = clampf (t, 0.0f, 1.0f);
        return { lerp (c.r, target.r, t), lerp (c.g, target.g, t), lerp (c.b, target.b, t) };
    }

    inline RGB whiten (RGB c, float t) noexcept { return toward (c, { 1.0f, 0.985f, 0.972f }, t); }

    /** Depth desaturation: a ribbon behind the core loses its colour into the cold volume. */
    inline RGB deepen (RGB c, float t) noexcept { return toward (c, { 0.176f, 0.196f, 0.373f }, t); }

    /**
        Smooth, slowly evolving 3D flow field. Each component is driven by the two
        other coordinates, which makes the field swirl (near-divergence-free) rather
        than push everything one way. Three noise reads per sample.
    */
    inline Vec3 flow (const ValueNoise& n, const Vec3& p, float t, float k) noexcept
    {
        const float x = p.x * k, y = p.y * k, z = p.z * k;
        return { n.noise (y + t * 0.37f,  z + 11.3f),
                 n.noise (z - t * 0.29f,  x + 27.7f),
                 n.noise (x + t * 0.23f,  y + 41.1f) };
    }
}

} // namespace am::ui
