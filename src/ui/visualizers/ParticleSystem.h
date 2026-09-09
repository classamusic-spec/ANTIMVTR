#pragma once

#include <array>
#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/Random.h"
#include "ValueNoise.h"

namespace am::ui
{

/**
    Preallocated particle sets for the ANTI-MATTER OBJECT:

      * Fragments — crystalline shards on inclined 3D orbits around the
        object (parallax: depth drives size, speed, brightness and whether a
        shard passes in front of or behind the body). Fracture activity
        gives them outward bursts; Decay keeps the motion alive longer;
        Magnet flattens the orbits into an aligned ring; Scatter jitters.
      * Sparks — small energy motes drifting in a noise flow field around
        the body; count follows Density and life.
      * Stars — the static environmental starfield with slow twinkle.

    All positions are in object units (1 = base radius R) and are converted
    to pixels by the visualizer. No allocation after construction.
*/
class ParticleSystem
{
public:
    static constexpr int kMaxFragments = 64;
    static constexpr int kMaxSparks    = 96;
    static constexpr int kMaxStars     = 150;
    static constexpr int kHistory      = 4;

    struct Fragment
    {
        float angle = 0.0f, speed = 0.0f, orbit = 1.0f, inclination = 0.0f, node = 0.0f;
        float size = 0.03f, spin = 0.0f, spinPhase = 0.0f, hue = 0.0f, seed = 0.0f;
        int   sides = 4;
        std::array<float, 6> vertexRadius {};
        // dynamic state
        float burst = 0.0f;          ///< extra radial offset from Fracture bursts
        float burstVelocity = 0.0f;
        float x = 0.0f, y = 0.0f, depth = 0.0f, scale = 1.0f;
        std::array<float, kHistory> hx {}, hy {};
        int historyHead = 0;
    };

    struct Spark
    {
        float angle = 0.0f, radius = 1.2f, phase = 0.0f, speed = 0.0f, size = 1.0f, hue = 0.0f;
        float x = 0.0f, y = 0.0f, brightness = 0.0f;
    };

    struct Star { float x, y, size, twinkle; };

    struct Env
    {
        float dt = 1.0f / 60.0f, time = 0.0f;
        float decay = 0.5f, magnet = 0.0f, scatter = 0.0f, tear = 0.0f, gravity = 0.5f, density = 0.5f;
        float fracture = 0.0f, life = 0.0f, pulse = 0.0f, tension = 0.5f, melt = 0.0f;
    };

    explicit ParticleSystem (uint32_t seed = 0xA11CEu)
    {
        Rng rng (seed);
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (auto& f : fragments)
        {
            f.angle = rng.nextFloat() * twoPi;
            f.speed = (0.06f + 0.16f * rng.nextFloat()) * (rng.chance (0.5f) ? 1.0f : -1.0f);
            f.orbit = 1.18f + 0.55f * rng.nextFloat() * rng.nextFloat() + 0.15f * rng.nextFloat();
            f.inclination = (0.35f + 0.9f * rng.nextFloat()) * (rng.chance (0.5f) ? 1.0f : -1.0f);
            f.node = rng.nextFloat() * twoPi;
            f.size = 0.018f + 0.045f * rng.nextFloat() * rng.nextFloat();
            f.spin = rng.nextBipolar() * 1.2f;
            f.spinPhase = rng.nextFloat() * twoPi;
            f.hue = rng.nextFloat();
            f.seed = rng.nextFloat() * 100.0f;
            f.sides = 3 + rng.nextInt (4);
            for (auto& vr : f.vertexRadius) vr = 0.55f + 0.45f * rng.nextFloat();
        }
        for (auto& s : sparks)
        {
            s.angle = rng.nextFloat() * twoPi;
            s.radius = 0.75f + 0.9f * rng.nextFloat();
            s.phase = rng.nextFloat() * twoPi;
            s.speed = (0.15f + 0.35f * rng.nextFloat()) * (rng.chance (0.6f) ? 1.0f : -1.0f);
            s.size = 0.6f + 1.4f * rng.nextFloat() * rng.nextFloat();
            s.hue = rng.nextFloat();
        }
        for (auto& st : stars)
            st = { rng.nextFloat(), rng.nextFloat(), 0.4f + 1.5f * rng.nextFloat() * rng.nextFloat(), rng.nextFloat() * twoPi };
    }

    /** Advances every fragment (object units). Call once per frame. */
    void updateFragments (const Env& e, const ValueNoise& noise, int activeCount) noexcept
    {
        const float persistence = 0.35f + 0.65f * e.decay;
        const float speedScale = (0.25f + 0.75f * persistence) * (0.55f + 0.45f * e.life) + 1.6f * e.fracture;
        const float burstDrag = std::exp (-e.dt * (4.5f - 3.6f * e.decay));
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < activeCount && i < kMaxFragments; ++i)
        {
            auto& f = fragments[(size_t) i];
            f.angle += f.speed * speedScale * e.dt;
            if (f.angle > twoPi) f.angle -= twoPi; else if (f.angle < 0.0f) f.angle += twoPi;

            // Fracture: impulsive outward bursts, persistence set by Decay.
            if (e.fracture > 0.02f)
            {
                const float kick = juce::jmax (0.0f, noise.noise (f.seed, e.time * 2.5f) - 0.35f);
                f.burstVelocity += kick * e.fracture * 6.0f * e.dt;
            }
            f.burstVelocity *= burstDrag;
            f.burst += f.burstVelocity * e.dt;
            f.burst -= f.burst * juce::jmax (0.0f, 1.0f - 0.85f * e.decay) * e.dt * 1.5f;
            f.burst = juce::jlimit (0.0f, 1.2f, f.burst);

            // 3D orbit → screen with parallax. Magnet aligns orbits into a flat ring; Gravity pulls them in.
            const float incl = f.inclination * (1.0f - 0.92f * e.magnet);
            const float orbit = (f.orbit + 0.45f * e.tear + f.burst) * (1.15f - 0.3f * e.gravity);
            const float px = std::cos (f.angle) * orbit;
            const float py = std::sin (f.angle) * orbit;
            const float yz = py * std::cos (incl);
            f.depth = py * std::sin (incl) / juce::jmax (0.001f, orbit);      // -1 .. 1
            const float jitter = e.scatter * 0.12f;
            const float jx = jitter * noise.noise (f.seed + e.time * 3.0f, 1.7f);
            const float jy = jitter * noise.noise (2.9f, f.seed + e.time * 3.0f);
            const float cn = std::cos (f.node), sn = std::sin (f.node);
            f.x = (px * cn - yz * sn) + jx;
            f.y = (px * sn + yz * cn) * (1.0f + 0.1f * e.melt) + jy + 0.18f * e.melt;
            f.scale = 1.0f + 0.4f * f.depth;

            f.hx[(size_t) f.historyHead] = f.x;
            f.hy[(size_t) f.historyHead] = f.y;
            f.historyHead = (f.historyHead + 1) % kHistory;
        }
    }

    /** Oldest recorded position of a fragment (for motion streaks). */
    static juce::Point<float> oldest (const Fragment& f) noexcept
    {
        return { f.hx[(size_t) f.historyHead], f.hy[(size_t) f.historyHead] };
    }

    /** Advances sparks in a noise flow field around the object. */
    void updateSparks (const Env& e, const ValueNoise& noise, int activeCount) noexcept
    {
        const float flow = 0.35f + 0.65f * e.life + e.fracture;
        for (int i = 0; i < activeCount && i < kMaxSparks; ++i)
        {
            auto& s = sparks[(size_t) i];
            s.angle += s.speed * flow * e.dt * (1.0f - 0.5f * e.magnet);
            const float n = noise.noise (std::cos (s.angle) * 1.5f + e.time * 0.2f, std::sin (s.angle) * 1.5f + s.phase);
            const float r = s.radius + 0.18f * n + 0.05f * std::sin (e.time * 1.7f + s.phase) + 0.25f * e.tear;
            const float stretch = 1.0f + 0.35f * (e.tension - 0.5f);
            s.x = std::cos (s.angle) * r * stretch;
            s.y = std::sin (s.angle) * r / stretch;
            s.brightness = juce::jlimit (0.0f, 1.0f, 0.35f + 0.65f * noise.noise (s.phase * 7.0f, e.time * (1.5f + 3.0f * e.life)));
        }
    }

    std::array<Fragment, kMaxFragments> fragments;
    std::array<Spark, kMaxSparks> sparks;
    std::array<Star, kMaxStars> stars;
};

} // namespace am::ui
