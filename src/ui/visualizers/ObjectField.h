#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ValueNoise.h"

namespace am::ui
{

/**
    Polar geometry of one lobe of the ANTI-MATTER OBJECT.

    A lobe is a closed outline sampled at kSamples angles. The radius profile
    is assembled from the eased engine state:

      Density  → number of undulations (noise frequency) and detail octaves
      Form     → smooth organic (noise) → faceted crystalline (polygon blend)
      Surface  → high-frequency roughness on the edge
      Bend     → limaçon-style asymmetric bulge
      Magnet   → k-fold symmetry (aligned "flower" harmonic, damped noise)
      Melt     → sagging, bottom-heavy profile
      Crush    → quantised (stepped) radius
      Fracture → chipped, jittering edge
      Tension  → elongation along a slowly rotating axis (applied to points)

    Everything is written into preallocated arrays; no heap use per frame.
*/
class ObjectField
{
public:
    static constexpr int kSamples = 128;

    struct Params
    {
        float density = 0.5f, form = 0.3f, surface = 0.2f, bend = 0.0f, magnet = 0.0f;
        float melt = 0.0f, crush = 0.0f, fracture = 0.0f, tension = 0.5f;
        float rotation = 0.0f;    ///< structural rotation (radians)
        float stretchAxis = 0.0f; ///< angle of the Tension axis
        float bendAxis = 0.0f;    ///< direction the Bend bulge points to
        float facets = 6.0f;      ///< polygon sides at Form = 1 (pitch driven)
        float symmetry = 4.0f;    ///< k-fold order used by Magnet
        float phase = 0.0f;       ///< noise phase (differs per shell / lobe)
        float time = 0.0f;
        float breathe = 1.0f;     ///< uniform scale (pulse + idle breathing)
    };

    struct Outline
    {
        std::array<juce::Point<float>, kSamples> points;
        std::array<juce::Point<float>, kSamples> normals;   ///< unit, outward
        std::array<float, kSamples> radius;                  ///< normalised (≈1)
        juce::Point<float> centre;
        float baseRadius = 1.0f;
    };

    /** Normalised radius profile (mean ≈ 1). */
    void computeProfile (const Params& p, const ValueNoise& noise, float* r, int n) const noexcept
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float formShape = smoothstep (0.15f, 0.95f, p.form) * (1.0f - 0.6f * p.melt);
        const float noiseFreq = 1.1f + 1.9f * p.density;
        const float noiseAmp  = (0.05f + 0.11f * p.density) * (1.0f - 0.55f * formShape) * (1.0f - 0.7f * p.magnet);
        const int   octaves   = p.density > 0.6f ? 3 : 2;
        const int   sides     = juce::jlimit (3, 12, (int) std::lround (p.facets));
        const float sector    = twoPi / (float) sides;
        const float apothem   = std::cos (juce::MathConstants<float>::pi / (float) sides);
        const float polyNorm  = 2.0f / (1.0f + apothem);
        const int   k         = juce::jlimit (2, 8, (int) std::lround (p.symmetry));
        const float slowT     = p.time * 0.11f;

        for (int i = 0; i < n; ++i)
        {
            const float theta = (float) i / (float) n * twoPi;
            const float ts = theta + p.rotation;             // structural angle (rotates with the object)

            // Organic undulation (seamless ring noise).
            float organic = noise.ring (ts, noiseFreq, slowT, p.phase, octaves) * noiseAmp;

            // Crystalline facets: regular polygon blended with a second, offset polygon for irregular crystals.
            float local = std::fmod (ts + juce::MathConstants<float>::pi * 4.0f, sector);
            float poly  = apothem / juce::jmax (0.35f, std::cos (local - sector * 0.5f)) * polyNorm - 1.0f;
            const float sector2 = twoPi / (float) (sides + 2);
            float local2 = std::fmod (ts + 0.37f + juce::MathConstants<float>::pi * 4.0f, sector2);
            float poly2  = std::cos (juce::MathConstants<float>::pi / (float) (sides + 2))
                           / juce::jmax (0.35f, std::cos (local2 - sector2 * 0.5f)) - 1.0f;
            const float crystal = (poly * 0.75f + poly2 * 0.35f) * 0.85f;

            float radius = 1.0f + organic * (1.0f - formShape) + crystal * formShape;

            // Magnet: aligned k-fold harmonic ("flower" symmetry).
            radius += p.magnet * 0.09f * std::cos ((float) k * ts);

            // Surface: fine roughness.
            if (p.surface > 0.01f)
                radius += p.surface * 0.035f * noise.ring (ts, 9.0f, p.time * 0.5f, p.phase + 3.3f, 2);

            // Bend: asymmetric bulge (limaçon) with a pinched third harmonic.
            if (p.bend > 0.001f)
                radius += p.bend * (0.20f * std::cos (theta - p.bendAxis) + 0.07f * std::cos (3.0f * (theta - p.bendAxis) + 1.0f));

            // Melt: sag toward the bottom, slight thinning at the top.
            if (p.melt > 0.001f)
            {
                const float down = juce::jmax (0.0f, std::sin (theta));
                radius += p.melt * (0.22f * down * down - 0.07f);
            }

            // Fracture: chipped, agitated edge.
            if (p.fracture > 0.001f)
                radius += p.fracture * 0.06f * noise.ring (theta, 14.0f, p.time * 3.0f, 7.7f, 1);

            // Crush: quantised radius steps.
            if (p.crush > 0.01f)
            {
                const float q = 40.0f - 34.0f * p.crush;
                radius = std::round (radius * q) / q;
            }

            r[i] = juce::jlimit (0.45f, 1.7f, radius) * p.breathe;
        }
    }

    /** Fills `out` with the outline of a lobe of base radius R at `centre`. */
    void buildOutline (const Params& p, const ValueNoise& noise, juce::Point<float> centre, float R, Outline& out) const noexcept
    {
        computeProfile (p, noise, out.radius.data(), kSamples);
        out.centre = centre;
        out.baseRadius = R;

        // Tension: elongate along the stretch axis.
        const float stretch = 1.0f + 0.42f * (p.tension - 0.5f) * 2.0f;
        const float sx = stretch, sy = 1.0f / juce::jmax (0.5f, stretch);
        const float ca = std::cos (p.stretchAxis), sa = std::sin (p.stretchAxis);
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < kSamples; ++i)
        {
            const float theta = (float) i / (float) kSamples * twoPi;
            const float x = std::cos (theta) * out.radius[(size_t) i] * R;
            const float y = std::sin (theta) * out.radius[(size_t) i] * R;
            // rotate into the stretch frame, scale, rotate back
            const float u = x * ca + y * sa, v = -x * sa + y * ca;
            const float us = u * sx, vs = v * sy;
            out.points[(size_t) i] = { centre.x + us * ca - vs * sa, centre.y + us * sa + vs * ca };
        }
        for (int i = 0; i < kSamples; ++i)
        {
            const auto& prev = out.points[(size_t) ((i + kSamples - 1) % kSamples)];
            const auto& next = out.points[(size_t) ((i + 1) % kSamples)];
            float nx = next.y - prev.y, ny = -(next.x - prev.x);
            const float len = std::sqrt (nx * nx + ny * ny);
            if (len > 1.0e-5f) { nx /= len; ny /= len; } else { nx = 1.0f; ny = 0.0f; }
            out.normals[(size_t) i] = { nx, ny };
        }
    }

    /** Writes the outline into a preallocated closed path (lineTo polygon; 128 samples is visually smooth). */
    static void toPath (const Outline& o, juce::Path& path, float scale = 1.0f) noexcept
    {
        path.clear();
        if (scale == 1.0f)
        {
            path.startNewSubPath (o.points[0]);
            for (int i = 1; i < kSamples; ++i) path.lineTo (o.points[(size_t) i]);
        }
        else
        {
            auto at = [&] (int i) { const auto& q = o.points[(size_t) i]; return juce::Point<float> (o.centre.x + (q.x - o.centre.x) * scale, o.centre.y + (q.y - o.centre.y) * scale); };
            path.startNewSubPath (at (0));
            for (int i = 1; i < kSamples; ++i) path.lineTo (at (i));
        }
        path.closeSubPath();
    }

    /** Point on the outline at a fractional sample index (wraps). */
    static juce::Point<float> pointAt (const Outline& o, float index, float scale = 1.0f) noexcept
    {
        index = std::fmod (index + (float) kSamples * 8.0f, (float) kSamples);
        const int i0 = (int) index, i1 = (i0 + 1) % kSamples;
        const float t = index - (float) i0;
        const auto a = o.points[(size_t) i0], b = o.points[(size_t) i1];
        juce::Point<float> p (a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
        if (scale != 1.0f) p = { o.centre.x + (p.x - o.centre.x) * scale, o.centre.y + (p.y - o.centre.y) * scale };
        return p;
    }

    static float smoothstep (float a, float b, float x) noexcept
    {
        const float t = juce::jlimit (0.0f, 1.0f, (x - a) / (b - a));
        return t * t * (3.0f - 2.0f * t);
    }
};

} // namespace am::ui
