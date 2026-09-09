#pragma once

#include <cstdint>
#include <cmath>

namespace am
{

/**
    Deterministic, allocation-free pseudo random generator (xoshiro128**).

    Every stochastic process in ANTI-MATR (Dust, Scatter, Chaos, Fracture
    probability, Mutation) must draw from a seeded Rng so that a fixed seed
    always reproduces the same behaviour.
*/
class Rng
{
public:
    explicit Rng (uint32_t seed = 0x9E3779B9u) noexcept { reseed (seed); }

    void reseed (uint32_t seed) noexcept
    {
        // SplitMix32 to expand the seed into four non-zero state words.
        uint32_t z = seed + 0x9E3779B9u;
        for (auto& w : s)
        {
            z += 0x9E3779B9u;
            uint32_t x = z;
            x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
            x = (x ^ (x >> 13)) * 0xC2B2AE35u;
            x ^= x >> 16;
            w = x | 1u;
        }
        for (int i = 0; i < 8; ++i) next();
    }

    inline uint32_t next() noexcept
    {
        const uint32_t result = rotl (s[1] * 5u, 7) * 9u;
        const uint32_t t = s[1] << 9;
        s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl (s[3], 11);
        return result;
    }

    /** Uniform float in [0, 1). */
    inline float nextFloat() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }

    /** Uniform float in [-1, 1). */
    inline float nextBipolar() noexcept { return nextFloat() * 2.0f - 1.0f; }

    /** Uniform integer in [0, n). */
    inline int nextInt (int n) noexcept { return n <= 0 ? 0 : (int) (next() % (uint32_t) n); }

    /** Uniform float in [lo, hi). */
    inline float nextRange (float lo, float hi) noexcept { return lo + (hi - lo) * nextFloat(); }

    /** Approximately Gaussian (mean 0, sigma 1) using the sum of 4 uniforms. Cheap and bounded. */
    inline float nextGaussian() noexcept
    {
        return (nextFloat() + nextFloat() + nextFloat() + nextFloat() - 2.0f) * 1.7320508f;
    }

    /** Returns true with probability p. */
    inline bool chance (float p) noexcept { return nextFloat() < p; }

private:
    static inline uint32_t rotl (uint32_t x, int k) noexcept { return (x << k) | (x >> (32 - k)); }
    uint32_t s[4] { 1, 2, 3, 4 };
};

/** Hash helper to derive stable per-object seeds from a master seed. */
inline uint32_t hashSeed (uint32_t seed, uint32_t salt) noexcept
{
    uint32_t x = seed ^ (salt * 0x9E3779B9u);
    x = (x ^ (x >> 16)) * 0x7FEB352Du;
    x = (x ^ (x >> 15)) * 0x846CA68Bu;
    return x ^ (x >> 16);
}

} // namespace am
