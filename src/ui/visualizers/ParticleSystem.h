#pragma once

#include <array>
#include <cmath>
#include "core/Random.h"
#include "LiquidOrganism.h"
#include "ValueNoise.h"

namespace am::ui
{

/**
    The two populations that float inside the sphere with the ANTI-MATTER object
    (VISUAL_SPEC §5). Both live in object space — 1 unit is the sphere radius —
    and are converted to pixels by the visualizer.

      Sparkles — fine bright motes scattered through the volume, twinkling as
                 they drift in and out of the light. A Fracture hit throws them
                 outward; the burst decays back over a couple of seconds.
      Bubbles  — larger translucent spheres with a rim highlight and a specular
                 dot, drifting slowly, scaled and dimmed by their depth.

    Everything is preallocated; nothing allocates after construction, and every
    position stays inside a bounded box however long the instrument runs.
*/
class ParticleSystem
{
public:
    static constexpr int kMaxSparkles = 130;
    static constexpr int kMaxBubbles  = 18;

    struct Sparkle
    {
        Vec3  home;                   ///< resting position in the ball
        float phase = 0.0f, rate = 1.0f, size = 1.0f, hue = 0.0f, seed = 0.0f;
        float burst = 0.0f, burstVel = 0.0f;
        Vec3  p;                      ///< live position (object space)
        float bright = 0.0f;
    };

    struct Bubble
    {
        Vec3  home;
        float radius = 0.05f, phase = 0.0f, rate = 0.4f, hue = 0.0f, seed = 0.0f;
        Vec3  p;
        float scale = 1.0f;
    };

    struct Env
    {
        float dt = 1.0f / 30.0f;
        float time = 0.0f, flowTime = 0.0f;
        float density = 0.5f, decay = 0.5f, tension = 0.5f, melt = 0.0f, scatter = 0.0f;
        float life = 0.0f, level = 0.0f, fracture = 0.0f, breathe = 1.0f;
    };

    explicit ParticleSystem (uint32_t seed = 0xA11CEu) noexcept { reseed (seed); }

    void reseed (uint32_t seed) noexcept
    {
        Rng rng (seed);
        constexpr float twoPi = 6.28318531f;
        for (auto& s : sparkles)
        {
            s.home = inBall (rng, 0.20f, 1.22f);
            s.phase = rng.nextFloat() * twoPi;
            s.rate = 0.25f + 0.9f * rng.nextFloat();
            s.size = 0.5f + 1.5f * rng.nextFloat() * rng.nextFloat();
            s.hue = rng.nextFloat();
            s.seed = rng.nextFloat() * 80.0f;
            s.burst = 0.0f;
            s.burstVel = 0.0f;
            s.p = s.home;
            s.bright = 0.0f;
        }
        for (auto& b : bubbles)
        {
            b.home = inBall (rng, 0.35f, 1.05f);
            b.radius = 0.030f + 0.052f * rng.nextFloat() * rng.nextFloat();
            b.phase = rng.nextFloat() * twoPi;
            b.rate = 0.10f + 0.22f * rng.nextFloat();
            b.hue = rng.nextFloat();
            b.seed = rng.nextFloat() * 60.0f;
            b.p = b.home;
            b.scale = 1.0f;
        }
    }

    /** How many sparkles are alive: DENSITY, playing state and Fracture all add. */
    static int sparkleCount (const Env& raw, int cap) noexcept
    {
        const Env e = sanitise (raw);
        const int n = 26 + (int) (e.density * 56.0f * (0.45f + 0.55f * e.life)) + (int) (e.fracture * 40.0f);
        const int top = cap < kMaxSparkles ? cap : kMaxSparkles;
        return (int) liquid::clampf ((float) n, 4.0f, (float) (top > 4 ? top : 4));
    }

    /** Bubbles are always few — they are the large, slow population. */
    static int bubbleCount (const Env& raw, int cap) noexcept
    {
        const Env e = sanitise (raw);
        const int n = 4 + (int) (e.density * 11.0f);
        const int top = cap < kMaxBubbles ? cap : kMaxBubbles;
        return (int) liquid::clampf ((float) n, 2.0f, (float) (top > 2 ? top : 2));
    }

    /** Advances the sparkles. `count` comes from sparkleCount(). */
    void updateSparkles (const Env& raw, const ValueNoise& noise, int count) noexcept
    {
        const Env e = sanitise (raw);
        const float drag = std::exp (-e.dt * (2.2f - 1.6f * e.decay));
        const float settle = 1.0f - std::exp (-e.dt * (0.9f - 0.55f * e.decay));
        const float drift = (0.16f + 0.20f * e.density) * (1.0f - 0.45f * e.tension);

        for (int i = 0; i < count && i < kMaxSparkles; ++i)
        {
            auto& s = sparkles[(size_t) i];

            if (e.fracture > 0.02f)
            {
                const float kick = e.fracture * 5.5f * liquid::clampf (noise.noise (s.seed, e.time * 2.3f) + 0.35f, 0.0f, 1.4f);
                s.burstVel += kick * e.dt;
            }
            s.burstVel *= drag;
            s.burst += s.burstVel * e.dt;
            s.burst -= s.burst * settle;
            s.burst = liquid::clampf (s.burst, 0.0f, 1.6f);

            const Vec3 f = liquid::flow (noise, s.home, e.flowTime + s.phase * 0.2f, 1.6f);
            Vec3 q = s.home + f * drift;
            q.y += e.melt * 0.28f;
            if (e.scatter > 0.001f)
                q += Vec3 { noise.noise (s.seed + 3.0f, e.time * 2.6f),
                            noise.noise (s.seed + 19.0f, e.time * 2.9f),
                            noise.noise (s.seed + 47.0f, e.time * 3.3f) } * (e.scatter * 0.18f);

            q *= (1.0f + s.burst) * e.breathe;
            s.p = { liquid::clampf (q.x, -5.0f, 5.0f), liquid::clampf (q.y, -5.0f, 5.0f), liquid::clampf (q.z, -5.0f, 5.0f) };

            // Twinkle: a mote passing in and out of the light.
            const float tw = 0.5f + 0.5f * std::sin (e.time * (1.4f + 3.4f * s.rate) + s.phase);
            const float slow = 0.5f + 0.5f * noise.noise (s.seed * 0.7f, e.time * 0.35f);
            s.bright = liquid::clampf ((0.18f + 0.82f * tw * slow) * (0.35f + 0.65f * e.life) + 0.5f * e.fracture, 0.0f, 1.0f);
        }
    }

    /** Advances the bubbles. */
    void updateBubbles (const Env& raw, const ValueNoise& noise, int count) noexcept
    {
        const Env e = sanitise (raw);
        for (int i = 0; i < count && i < kMaxBubbles; ++i)
        {
            auto& b = bubbles[(size_t) i];
            const Vec3 f = liquid::flow (noise, b.home, e.flowTime * 0.6f + b.phase * 0.15f, 1.1f);
            Vec3 q = b.home + f * (0.20f * (1.0f - 0.4f * e.tension));
            q.y += 0.06f * std::sin (e.time * b.rate + b.phase) - e.melt * 0.12f;
            q *= e.breathe;
            b.p = { liquid::clampf (q.x, -4.0f, 4.0f), liquid::clampf (q.y, -4.0f, 4.0f), liquid::clampf (q.z, -4.0f, 4.0f) };
            b.scale = 1.0f + 0.35f * b.p.z;
        }
    }

    const std::array<Sparkle, kMaxSparkles>& sparkleArray() const noexcept { return sparkles; }
    const std::array<Bubble, kMaxBubbles>& bubbleArray() const noexcept { return bubbles; }

    static Env sanitise (const Env& e) noexcept
    {
        Env o;
        o.dt       = liquid::clean (e.dt, 1.0f / 240.0f, 0.25f, 1.0f / 30.0f);
        o.time     = liquid::clean (e.time, -1.0e6f, 1.0e6f, 0.0f);
        o.flowTime = liquid::clean (e.flowTime, -1.0e6f, 1.0e6f, 0.0f);
        o.density  = liquid::clean (e.density, 0.0f, 1.0f, 0.5f);
        o.decay    = liquid::clean (e.decay, 0.0f, 1.0f, 0.5f);
        o.tension  = liquid::clean (e.tension, 0.0f, 1.0f, 0.5f);
        o.melt     = liquid::clean (e.melt, 0.0f, 1.0f, 0.0f);
        o.scatter  = liquid::clean (e.scatter, 0.0f, 1.0f, 0.0f);
        o.life     = liquid::clean (e.life, 0.0f, 1.0f, 0.0f);
        o.level    = liquid::clean (e.level, 0.0f, 1.0f, 0.0f);
        o.fracture = liquid::clean (e.fracture, 0.0f, 2.0f, 0.0f);
        o.breathe  = liquid::clean (e.breathe, 0.4f, 2.0f, 1.0f);
        return o;
    }

private:
    static Vec3 inBall (Rng& rng, float rMin, float rMax) noexcept
    {
        for (int attempt = 0; attempt < 8; ++attempt)
        {
            const Vec3 v { rng.nextBipolar(), rng.nextBipolar(), rng.nextBipolar() };
            const float l = v.length();
            if (l > 0.05f && l <= 1.0f)
                return normalised (v) * (rMin + (rMax - rMin) * l);
        }
        return { rMin, 0.0f, 0.0f };
    }

    std::array<Sparkle, kMaxSparkles> sparkles {};
    std::array<Bubble, kMaxBubbles> bubbles {};
};

} // namespace am::ui
