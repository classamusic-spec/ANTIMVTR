#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include "core/Random.h"

namespace am::ui
{

/**
    Smooth, deterministic 2D value noise with fractal (fBm) summation.

    Used by the ANTI-MATTER OBJECT for organic deformation, filament wobble,
    fragment drift and flicker. Allocation-free: the permutation table lives
    inline. Seeded through am::Rng so a fixed seed always reproduces the same
    field. Output of noise() is in [-1, 1]; fbm() is normalised to roughly
    the same range.
*/
class ValueNoise
{
public:
    explicit ValueNoise (uint32_t seed = 0x5EEDu) noexcept { reseed (seed); }

    void reseed (uint32_t seed) noexcept
    {
        Rng rng (seed);
        for (int i = 0; i < 256; ++i) perm[(size_t) i] = (uint8_t) i;
        for (int i = 255; i > 0; --i)
        {
            const int j = rng.nextInt (i + 1);
            std::swap (perm[(size_t) i], perm[(size_t) j]);
        }
        for (int i = 0; i < 256; ++i)
        {
            perm[(size_t) (i + 256)] = perm[(size_t) i];
            values[(size_t) i] = rng.nextBipolar();
        }
    }

    /** Smooth value noise, C2 continuous (quintic fade). Range [-1, 1]. */
    float noise (float x, float y) const noexcept
    {
        const float fx = std::floor (x), fy = std::floor (y);
        const int ix = (int) fx & 255, iy = (int) fy & 255;
        const float tx = fade (x - fx), ty = fade (y - fy);

        const float v00 = lattice (ix,     iy);
        const float v10 = lattice (ix + 1, iy);
        const float v01 = lattice (ix,     iy + 1);
        const float v11 = lattice (ix + 1, iy + 1);

        const float a = v00 + (v10 - v00) * tx;
        const float b = v01 + (v11 - v01) * tx;
        return a + (b - a) * ty;
    }

    /** Fractal Brownian motion: `octaves` layers, each doubling frequency and halving amplitude. */
    float fbm (float x, float y, int octaves) const noexcept
    {
        float sum = 0.0f, amp = 1.0f, norm = 0.0f;
        for (int o = 0; o < octaves; ++o)
        {
            sum += noise (x, y) * amp;
            norm += amp;
            amp *= 0.5f;
            x = x * 2.03f + 17.31f;
            y = y * 1.97f + 9.17f;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    /** Noise sampled on a circle: seamless in `angle`, animated by `time`. */
    float ring (float angle, float frequency, float time, float phase, int octaves) const noexcept
    {
        const float cx = std::cos (angle) * frequency + phase;
        const float cy = std::sin (angle) * frequency + time;
        return fbm (cx, cy, octaves);
    }

private:
    static float fade (float t) noexcept { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

    float lattice (int x, int y) const noexcept
    {
        return values[perm[(size_t) (perm[(size_t) (x & 255)] + (y & 255))]];
    }

    std::array<uint8_t, 512> perm {};
    std::array<float, 256> values {};
};

} // namespace am::ui
