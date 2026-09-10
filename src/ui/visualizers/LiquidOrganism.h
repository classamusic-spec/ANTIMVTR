#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include "core/Random.h"
#include "ValueNoise.h"

namespace am::ui
{

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

//==============================================================================
/** One sample along a ribbon, in object space (sphere radius 1). */
struct RibbonSample
{
    Vec3  p;                  ///< position, object space
    float width = 0.0f;       ///< half-width in object units, already tapered
    float u = 0.0f;           ///< 0..1 along the ribbon (colour and taper parameter)
    float bright = 1.0f;      ///< 0..1 emission along the length
};

/** A run of consecutive samples that lies wholly in front of or wholly behind the core. */
struct RibbonSpan
{
    int   ribbon = 0;
    int   first = 0;
    int   count = 0;
    float meanZ = 0.0f;       ///< sort key — painter's algorithm, back to front
    bool  angular = false;    ///< Crush: draw with straight, mitred joins
};

//==============================================================================
/**
    THE ANTI-MATTER OBJECT — geometry of a liquid-light organism (VISUAL_SPEC §5).

    The organism is a bundle of wide tapering ribbons that travel on and around an
    implicit unit sphere. A ribbon is a great-circle arc in its own frame, pushed
    around by a slowly evolving 3D flow field so the whole mass advects like a
    fluid, and re-weighted so it thins where it is stretched and bulges where it
    slows. Everything is a pure function of (ribbon index, Params, noise), so a
    seed reproduces exactly and nothing can drift or blow up over hours of running.

    Engine reactions live here rather than in the painter:

      Density  → ribbon count and sample count
      Tension  → orbits tighten toward the sphere, ribbons thin
      Surface  → the two edges roughen independently
      Mass     → the core opens (see `coreRadius`)
      Bend     → the ribbon curves toward a slowly turning direction
      Melt     → the tail sags and runs downward, widening
      Tear     → a ribbon opens a gap and the two halves drift apart
      Magnet   → ribbons are pulled onto a few shared paths
      Gravity  → the whole bundle is pulled in or pushed out
      Scatter  → per-sample jitter
      Freeze   → the caller stops advancing `flowTime`; drift stops dead
      Crush    → positions quantise onto a coarse lattice, joins go angular
*/
class LiquidOrganism
{
public:
    static constexpr int kMaxRibbons = 26;
    static constexpr int kMaxSamples = 48;
    static constexpr int kMaxSpans   = kMaxRibbons * 4;
    static constexpr int kAttractors = 3;

    struct Params
    {
        float time = 0.0f;          ///< wall clock (drives wobble and twinkle)
        float flowTime = 0.0f;      ///< flow-field clock — held still by Freeze
        float rotation = 0.0f;      ///< yaw of the whole organism
        float pitch = 0.34f;        ///< fixed tilt so the sphere never reads flat

        float density = 0.5f, form = 0.3f, mass = 0.4f, tension = 0.5f, decay = 0.5f, surface = 0.2f;
        float bend = 0.0f, melt = 0.0f, tear = 0.0f, magnet = 0.0f, gravity = 0.5f, scatter = 0.0f, crush = 0.0f;

        float level = 0.0f;         ///< eased audio level 0..1
        float energy = 0.0f;        ///< note envelope 0..1
        float life = 0.0f;          ///< 0 idle → 1 playing
        float fracture = 0.0f;      ///< eased fracture activity
        float breathe = 1.0f;       ///< uniform scale (idle breathing + audio pulse)
    };

    explicit LiquidOrganism (uint32_t seed = 0x11B0Fu) noexcept { reseed (seed); }

    void reseed (uint32_t seed) noexcept
    {
        Rng rng (seed);
        constexpr float twoPi = 6.28318531f;
        for (int i = 0; i < kMaxRibbons; ++i)
        {
            auto& r = ribbons[(size_t) i];
            // A random great circle: pick a normal, then two perpendicular axes in its plane.
            const Vec3 n = normalised ({ rng.nextBipolar(), rng.nextBipolar() * 0.85f, rng.nextBipolar() },
                                       { 0.0f, 1.0f, 0.0f });
            r.normal = n;
            const Vec3 seedUp = std::abs (n.y) > 0.9f ? Vec3 { 1.0f, 0.0f, 0.0f } : Vec3 { 0.0f, 1.0f, 0.0f };
            r.u = normalised (cross (seedUp, n));
            r.v = normalised (cross (n, r.u));
            r.phase = rng.nextFloat() * twoPi;
            r.drift = (0.055f + 0.10f * rng.nextFloat()) * (rng.chance (0.5f) ? 1.0f : -1.0f);
            r.span = 1.75f + 2.45f * rng.nextFloat();               // radians of arc travelled
            r.radius = 0.58f + 0.46f * rng.nextFloat();
            // Golden-ratio spread: any prefix of the ribbons still covers the whole
            // ramp, so the mass stays iridescent at every DENSITY setting.
            r.hue = (float) i * 0.61803399f + 0.06f * rng.nextFloat();
            r.hue -= std::floor (r.hue);
            r.hueSpan = 0.20f + 0.26f * rng.nextFloat();
            r.width = 0.094f + 0.078f * rng.nextFloat();
            r.wobbleFreq = 3.0f + 5.0f * rng.nextFloat();
            r.wobblePhase = rng.nextFloat() * twoPi;
            r.seed = rng.nextFloat() * 90.0f;
            r.tearVictim = (i % 3) == 1;
            r.attractor = i % kAttractors;
        }
        for (int a = 0; a < kAttractors; ++a)
        {
            const Vec3 n = normalised ({ rng.nextBipolar(), rng.nextBipolar() * 0.6f, rng.nextBipolar() },
                                       { 0.0f, 1.0f, 0.0f });
            attractorNormal[(size_t) a] = n;
            const Vec3 seedUp = std::abs (n.y) > 0.9f ? Vec3 { 1.0f, 0.0f, 0.0f } : Vec3 { 0.0f, 1.0f, 0.0f };
            attractorU[(size_t) a] = normalised (cross (seedUp, n));
            attractorV[(size_t) a] = normalised (cross (n, attractorU[(size_t) a]));
        }
    }

    /** Ribbon count follows Shape DENSITY (and a little of the note energy). */
    static int ribbonCount (const Params& p) noexcept
    {
        const float d = liquid::clean (p.density, 0.0f, 1.0f, 0.5f);
        const float life = liquid::clean (p.life, 0.0f, 1.0f, 0.0f);
        const int n = 10 + (int) (d * 13.0f) + (int) (life * 2.0f);
        return n < 4 ? 4 : (n > kMaxRibbons ? kMaxRibbons : n);
    }

    /** Samples per ribbon: enough for a smooth curve, fewer when Crush makes it angular. */
    static int sampleCount (const Params& p) noexcept
    {
        const float d = liquid::clean (p.density, 0.0f, 1.0f, 0.5f);
        const float crush = liquid::clean (p.crush, 0.0f, 1.0f, 0.0f);
        int n = 28 + (int) (d * 13.0f);
        if (crush > 0.05f) n = (int) (n * (1.0f - 0.55f * crush));
        return n < 8 ? 8 : (n > kMaxSamples ? kMaxSamples : n);
    }

    /** Radius of the dark organic core: it opens with MASS and closes as the object melts. */
    static float coreRadius (const Params& p) noexcept
    {
        const float mass = liquid::clean (p.mass, 0.0f, 1.0f, 0.4f);
        const float melt = liquid::clean (p.melt, 0.0f, 1.0f, 0.0f);
        const float level = liquid::clean (p.level, 0.0f, 1.0f, 0.0f);
        return liquid::clampf (0.20f + 0.30f * mass - 0.06f * melt + 0.02f * level, 0.11f, 0.55f);
    }

    /** The core silhouette: a slowly deforming lobed blob, never a circle. */
    float coreProfile (float angle, const Params& p, const ValueNoise& n) const noexcept
    {
        if (! std::isfinite (angle)) angle = 0.0f;
        const float t = liquid::clean (p.flowTime, -1.0e6f, 1.0e6f, 0.0f) * 1.6f;
        const float mass = liquid::clean (p.mass, 0.0f, 1.0f, 0.4f);
        const float melt = liquid::clean (p.melt, 0.0f, 1.0f, 0.0f);
        const float lobes = 2.0f + 1.6f * mass;
        const float wobble = n.ring (angle, 1.7f, t * 0.13f, 4.4f, 2) * (0.13f + 0.06f * mass);
        const float fine = n.ring (angle, 5.5f, t * 0.21f, 19.0f, 1) * 0.035f;
        const float sag = melt * 0.16f * liquid::clampf (std::sin (angle), 0.0f, 1.0f);
        const float lobe = 0.05f * std::cos (lobes * angle + t * 0.17f);
        return liquid::clampf (1.0f + wobble + fine + lobe + sag, 0.55f, 1.55f);
    }

    /**
        Builds one ribbon into `out`. Returns how many samples were written
        (`sampleCount(p)`, clipped to `maxOut`). Output is always finite and every
        component stays inside a bounded box, whatever the parameters do.
    */
    int buildRibbon (int index, const Params& raw, const ValueNoise& noise,
                     RibbonSample* out, int maxOut) const noexcept
    {
        if (out == nullptr || maxOut <= 1) return 0;

        const Params p = sanitise (raw);
        const int n = sampleCount (p) < maxOut ? sampleCount (p) : maxOut;
        const auto& r = ribbons[(size_t) (((index % kMaxRibbons) + kMaxRibbons) % kMaxRibbons)];

        // ---- Frame: Magnet pulls the ribbon's own great circle onto a shared one.
        const int a = r.attractor;
        Vec3 axisU = mix (r.u, attractorU[(size_t) a], p.magnet * 0.92f);
        Vec3 axisV = mix (r.v, attractorV[(size_t) a], p.magnet * 0.92f);
        axisU = normalised (axisU, r.u);
        axisV = normalised (axisV - axisU * dot (axisU, axisV), r.v);

        // ---- Orbit: Tension pulls every ribbon in onto the same tight shell.
        const float baseRadius = liquid::lerp (r.radius, 0.70f, p.tension * 0.85f)
                                 * (1.0f - 0.26f * (p.gravity - 0.5f) * 2.0f);
        const float arcSpan = r.span * (1.0f - 0.30f * p.tension) * (1.0f + 0.35f * p.bend);
        const float phase = r.phase + p.flowTime * r.drift * (0.45f + 0.55f * p.life);

        // ---- Flow: the field the ribbon advects through, and how hard it pushes.
        const float flowAmp = (0.10f + 0.13f * p.density) * (1.0f - 0.55f * p.tension) * (1.0f + 0.9f * p.melt);
        const float flowScale = 1.25f + 1.6f * p.density;

        // ---- Bend: a slowly turning direction the ribbon leans into.
        const float motion = p.flowTime * 1.6f;
        const Vec3 bendDir = normalised ({ std::cos (motion * 0.11f + (float) index * 0.7f),
                                           0.42f * std::sin (motion * 0.09f),
                                           std::sin (motion * 0.11f + (float) index * 0.7f) });

        // ---- Tear: this ribbon opens a gap in the middle and the halves drift apart.
        const float tearOpen = r.tearVictim ? liquid::smoothstep (0.06f, 0.85f, p.tear) : 0.0f;
        const Vec3 tearDir = normalised (cross (axisU, axisV), { 0.0f, 0.0f, 1.0f });

        const float quant = p.crush > 0.02f ? liquid::lerp (26.0f, 3.4f, p.crush) : 0.0f;
        const float widthBase = r.width * (1.0f - 0.40f * p.tension) * (0.82f + 0.34f * p.density)
                                * (1.0f + 0.5f * p.melt) * (0.88f + 0.30f * p.level);

        // ---- Pass 1: positions.
        for (int i = 0; i < n; ++i)
        {
            const float s = (float) i / (float) (n - 1);
            const float ang = phase + (s - 0.5f) * arcSpan;

            // The base path: a circle in the ribbon's own plane, radius modulated so it
            // swells in the middle rather than hugging a perfect sphere.
            const float shellR = baseRadius * (1.0f + 0.13f * std::sin (s * 3.14159265f) - 0.05f * p.tension);
            Vec3 q = axisU * (std::cos (ang) * shellR) + axisV * (std::sin (ang) * shellR);

            // Bend: lean the middle of the ribbon toward the bend direction.
            if (p.bend > 0.001f)
                q += bendDir * (p.bend * 0.42f * std::sin (3.14159265f * s));

            // Advection through the slowly evolving flow field.
            const Vec3 f = liquid::flow (noise, q, p.flowTime, flowScale);
            q += f * (flowAmp * (0.55f + 0.45f * std::sin (3.14159265f * s)));

            // Tear: push the two halves apart along the ribbon's plane normal.
            if (tearOpen > 0.0f)
            {
                const float side = s < 0.5f ? -1.0f : 1.0f;
                q += tearDir * (side * tearOpen * 0.34f);
                q += axisU * (side * tearOpen * 0.10f);
            }

            // Melt: the tail sags and runs downward, more the further along it is.
            if (p.melt > 0.001f)
                q.y += p.melt * (0.34f * s * s + 0.10f);

            // Scatter: per-sample jitter, high frequency so it reads as agitation.
            if (p.scatter > 0.001f)
            {
                const float js = r.seed + (float) i * 3.7f;
                q += Vec3 { noise.noise (js, motion * 3.1f),
                            noise.noise (js + 31.0f, motion * 2.7f),
                            noise.noise (js + 67.0f, motion * 3.5f) } * (p.scatter * 0.17f);
            }

            // Fracture blows the whole bundle outward for the length of the hit.
            if (p.fracture > 0.001f)
                q *= 1.0f + 0.16f * p.fracture;

            // Crush: quantise onto a coarse lattice — the fluid goes angular.
            if (quant > 0.0f)
            {
                q.x = std::round (q.x * quant) / quant;
                q.y = std::round (q.y * quant) / quant;
                q.z = std::round (q.z * quant) / quant;
            }

            q *= p.breathe;

            out[i].p = { liquid::clampf (q.x, -4.0f, 4.0f),
                         liquid::clampf (q.y, -4.0f, 4.0f),
                         liquid::clampf (q.z, -4.0f, 4.0f) };
            out[i].u = s;
        }

        // ---- Pass 2: width from taper × local stretch, plus a surface-tension wobble.
        float meanStep = 0.0f;
        for (int i = 1; i < n; ++i) meanStep += (out[i].p - out[i - 1].p).length();
        meanStep = meanStep > 1.0e-5f ? meanStep / (float) (n - 1) : 1.0e-5f;

        for (int i = 0; i < n; ++i)
        {
            const float s = out[i].u;

            // Taper: nothing at the ends, swelling through the middle. The second term
            // is the swell itself — without it the profile is a flat-topped strip and
            // the ribbon reads as a stroked polyline, which is exactly what it is not.
            const float arch = liquid::clampf (std::sin (3.14159265f * s), 0.0f, 1.0f);
            const float taper = std::pow (arch, 0.80f) * (0.66f + 0.52f * arch * arch);

            // Incompressibility: stretched here → thin; slowed here → bulge.
            const int i0 = i > 0 ? i - 1 : 0, i1 = i < n - 1 ? i + 1 : n - 1;
            const float localStep = (out[i1].p - out[i0].p).length() / (float) (i1 - i0 > 0 ? i1 - i0 : 1);
            const float stretch = liquid::clampf (meanStep / (localStep > 1.0e-6f ? localStep : 1.0e-6f), 0.45f, 1.9f);

            // Surface tension: a slow travelling wobble along the ribbon.
            const float wobble = 1.0f + 0.11f * std::sin (s * r.wobbleFreq * 6.2831853f
                                                          + motion * (1.3f + 0.9f * p.life) + r.wobblePhase);

            out[i].width = liquid::clampf (widthBase * taper * std::pow (stretch, 0.7f) * wobble, 0.0f, 0.70f);

            // Brightness along the length: a moving hot spot, plus the note envelope.
            const float travel = 0.5f + 0.5f * std::sin (s * 6.2831853f * 1.15f - motion * (0.7f + 1.4f * p.life) + r.seed);
            const float slow = 0.5f + 0.5f * std::sin (s * 6.2831853f * 0.37f + motion * 0.21f + r.wobblePhase);
            out[i].bright = liquid::clampf (0.34f + 0.50f * travel * (0.40f + 0.60f * slow)
                                            + 0.26f * p.energy + 0.20f * p.level, 0.0f, 1.35f);
        }

        return n;
    }

    /** The hue at position `s` along ribbon `index` — each ribbon starts elsewhere on the ramp. */
    float ribbonHue (int index, float s, float drift) const noexcept
    {
        const auto& r = ribbons[(size_t) (((index % kMaxRibbons) + kMaxRibbons) % kMaxRibbons)];
        return r.hue + drift + liquid::clampf (s, 0.0f, 1.0f) * r.hueSpan;
    }

    bool isAngular (const Params& p) const noexcept { return liquid::clean (p.crush, 0.0f, 1.0f, 0.0f) > 0.12f; }

    /**
        Splits a built ribbon into runs that lie wholly behind (z < 0) or wholly in
        front of the core, so the painter can draw back runs, then the core, then
        front runs. Runs share one sample at each cut so the ribbon stays continuous.

        A ribbon is only cut where it actually crosses *behind the core* — that is,
        within `splitRadius` of the axis. A ribbon that passes the z = 0 plane out at
        the silhouette needs no cut, and cutting it there would leave a blunt chisel
        end in the middle of a strand that should read as one continuous ribbon.

        Returns the number of spans appended to `spans`.
    */
    static int splitByDepth (int ribbonIndex, const RibbonSample* s, int count, bool angular,
                             float splitRadius, RibbonSpan* spans, int maxSpans) noexcept
    {
        if (s == nullptr || spans == nullptr || count < 2 || maxSpans <= 0) return 0;

        const float r2 = splitRadius * splitRadius;
        int written = 0;
        int start = 0;
        bool front = s[0].p.z >= 0.0f;

        auto emit = [&] (int from, int to)
        {
            const int len = to - from + 1;
            if (len < 2 || written >= maxSpans) return;
            float sum = 0.0f;
            for (int i = from; i <= to; ++i) sum += s[i].p.z;
            spans[written++] = { ribbonIndex, from, len, sum / (float) len, angular };
        };

        for (int i = 1; i < count; ++i)
        {
            const bool f = s[i].p.z >= 0.0f;
            const float lateral = s[i].p.x * s[i].p.x + s[i].p.y * s[i].p.y;
            if (f != front && lateral < r2)
            {
                // The crossing sample ends this run and begins the next, so the
                // ribbon reads as one continuous strand across the cut.
                emit (start, i);
                start = i;
                front = f;
            }
            else if (f != front)
            {
                front = f;                // crossed out at the silhouette: no cut needed
            }
        }
        emit (start, count - 1);
        return written;
    }

    /** Insertion sort, back to front. `n` is small (a few dozen) and almost sorted. */
    static void sortByDepth (RibbonSpan* spans, int n) noexcept
    {
        for (int i = 1; i < n; ++i)
        {
            const RibbonSpan key = spans[i];
            int j = i - 1;
            while (j >= 0 && spans[j].meanZ > key.meanZ) { spans[j + 1] = spans[j]; --j; }
            spans[j + 1] = key;
        }
    }

    /** Conditions every parameter: NaN and infinity become the safe default, everything clamps. */
    static Params sanitise (const Params& p) noexcept
    {
        Params o;
        o.time     = liquid::clean (p.time, -1.0e6f, 1.0e6f, 0.0f);
        o.flowTime = liquid::clean (p.flowTime, -1.0e6f, 1.0e6f, 0.0f);
        o.rotation = liquid::clean (p.rotation, -1.0e4f, 1.0e4f, 0.0f);
        o.pitch    = liquid::clean (p.pitch, -1.5f, 1.5f, 0.24f);
        o.density  = liquid::clean (p.density, 0.0f, 1.0f, 0.5f);
        o.form     = liquid::clean (p.form, 0.0f, 1.0f, 0.3f);
        o.mass     = liquid::clean (p.mass, 0.0f, 1.0f, 0.4f);
        o.tension  = liquid::clean (p.tension, 0.0f, 1.0f, 0.5f);
        o.decay    = liquid::clean (p.decay, 0.0f, 1.0f, 0.5f);
        o.surface  = liquid::clean (p.surface, 0.0f, 1.0f, 0.2f);
        o.bend     = liquid::clean (p.bend, 0.0f, 1.0f, 0.0f);
        o.melt     = liquid::clean (p.melt, 0.0f, 1.0f, 0.0f);
        o.tear     = liquid::clean (p.tear, 0.0f, 1.0f, 0.0f);
        o.magnet   = liquid::clean (p.magnet, 0.0f, 1.0f, 0.0f);
        o.gravity  = liquid::clean (p.gravity, 0.0f, 1.0f, 0.5f);
        o.scatter  = liquid::clean (p.scatter, 0.0f, 1.0f, 0.0f);
        o.crush    = liquid::clean (p.crush, 0.0f, 1.0f, 0.0f);
        o.level    = liquid::clean (p.level, 0.0f, 1.0f, 0.0f);
        o.energy   = liquid::clean (p.energy, 0.0f, 1.0f, 0.0f);
        o.life     = liquid::clean (p.life, 0.0f, 1.0f, 0.0f);
        o.fracture = liquid::clean (p.fracture, 0.0f, 2.0f, 0.0f);
        o.breathe  = liquid::clean (p.breathe, 0.4f, 2.0f, 1.0f);
        return o;
    }

private:
    struct Ribbon
    {
        Vec3 normal, u, v;
        float phase = 0.0f, drift = 0.0f, span = 3.0f, radius = 0.9f;
        float hue = 0.0f, hueSpan = 0.3f, width = 0.08f;
        float wobbleFreq = 4.0f, wobblePhase = 0.0f, seed = 0.0f;
        bool  tearVictim = false;
        int   attractor = 0;
    };

    std::array<Ribbon, kMaxRibbons> ribbons {};
    std::array<Vec3, kAttractors> attractorNormal {}, attractorU {}, attractorV {};
};

} // namespace am::ui
