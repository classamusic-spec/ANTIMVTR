#pragma once

#include <cmath>
#include <cstdint>

namespace am
{

/** Fast tanh approximation (Padé), accurate to ~1e-3 within ±4. */
inline float fastTanh (float x) noexcept
{
    if (x >  4.97f) return  1.0f;
    if (x < -4.97f) return -1.0f;
    const float x2 = x * x;
    return x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2))) /
               (135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f)));
}

/** Soft clipper with unity slope at 0, saturating toward ±1. */
inline float softClip (float x) noexcept
{
    return fastTanh (x);
}

/** Cheap sine approximation for phase in [0, 1). Max error ~1e-3. Use for LFO/visual, not precision oscillators. */
inline float fastSin01 (float phase01) noexcept
{
    float x = phase01 - 0.5f;                     // -0.5 .. 0.5
    x = x - std::floor (x + 0.5f);                // wrap
    const float y = x * (8.0f - 16.0f * std::abs (x)); // parabola ~ -sin(2*pi*x)... scaled
    return -(0.225f * (y * std::abs (y) - y) + y);
}

/** 2^x approximation for pitch conversions where a few cents error are acceptable. */
inline float fastPow2 (float x) noexcept
{
    const int   ipart = (int) std::floor (x);
    const float fpart = x - (float) ipart;
    const float p = 1.0f + fpart * (0.6931472f + fpart * (0.2402265f + fpart * (0.0555041f + fpart * 0.0096181f)));
    return std::ldexp (p, ipart);
}

} // namespace am
