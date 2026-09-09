#pragma once

#include "Types.h"

/**
    Small helpers that keep the audio thread safe: denormal handling,
    non-finite scrubbing, DC blocking, and tiny buffers.
*/
namespace am
{

/** Flushes a possibly non-finite value to zero. Returns true if it had to. */
inline bool scrubNonFinite (float& v) noexcept
{
    if (! std::isfinite (v)) { v = 0.0f; return true; }
    return false;
}

/** Scans a buffer, zeroes non-finite samples and returns how many were found. */
inline int scrubBuffer (float* data, int numSamples) noexcept
{
    int bad = 0;
    for (int i = 0; i < numSamples; ++i)
        if (! std::isfinite (data[i])) { data[i] = 0.0f; ++bad; }
    return bad;
}

/** Fast check whether any value in a buffer is non-finite. */
inline bool containsNonFinite (const float* data, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
        if (! std::isfinite (data[i])) return true;
    return false;
}

/** Adds a minuscule alternating offset to kill denormals in recursive filters. */
struct AntiDenormal
{
    float value = 1.0e-18f;
    inline float next() noexcept { value = -value; return value; }
};

/** Simple first-order DC blocker. */
struct DCBlocker
{
    void prepare (double sampleRate, float cutoffHz = 5.0f) noexcept
    {
        const double w = kTwoPi * (double) cutoffHz / sampleRate;
        R = (float) (1.0 - w);
        reset();
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

    inline float process (float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x; y1 = y;
        return y;
    }

    float R = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

/** Peak / RMS meter accumulator for a block of samples. */
struct LevelMeter
{
    float peak = 0.0f;
    float rms  = 0.0f;

    void measure (const float* data, int n) noexcept
    {
        float p = 0.0f; double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (data[i]);
            p = a > p ? a : p;
            sum += (double) data[i] * (double) data[i];
        }
        peak = p;
        rms  = n > 0 ? (float) std::sqrt (sum / (double) n) : 0.0f;
    }

    void measureStereo (const float* l, const float* r, int n) noexcept
    {
        float p = 0.0f; double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const float al = std::abs (l[i]), ar = std::abs (r[i]);
            p = al > p ? al : p; p = ar > p ? ar : p;
            sum += 0.5 * ((double) l[i] * (double) l[i] + (double) r[i] * (double) r[i]);
        }
        peak = p;
        rms  = n > 0 ? (float) std::sqrt (sum / (double) n) : 0.0f;
    }
};

/** A single-writer / single-reader ring of audio samples used for visualization taps.
    Torn reads are acceptable: this feeds displays, never DSP. */
template <int Capacity>
class AudioTapRing
{
public:
    static_assert ((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

    void clear() noexcept
    {
        std::fill (dataL.begin(), dataL.end(), 0.0f);
        std::fill (dataR.begin(), dataR.end(), 0.0f);
        writePos.store (0, std::memory_order_relaxed);
    }

    void push (const float* l, const float* r, int n) noexcept
    {
        uint32_t w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const uint32_t idx = w & (Capacity - 1);
            dataL[idx] = l[i];
            dataR[idx] = r != nullptr ? r[i] : l[i];
            ++w;
        }
        writePos.store (w, std::memory_order_release);
    }

    /** Copies the most recent `n` samples (n <= Capacity) into dest arrays. */
    void readLatest (float* l, float* r, int n) const noexcept
    {
        const uint32_t w = writePos.load (std::memory_order_acquire);
        const uint32_t start = w - (uint32_t) n;
        for (int i = 0; i < n; ++i)
        {
            const uint32_t idx = (start + (uint32_t) i) & (Capacity - 1);
            l[i] = dataL[idx];
            if (r != nullptr) r[i] = dataR[idx];
        }
    }

    uint32_t totalWritten() const noexcept { return writePos.load (std::memory_order_acquire); }

    static constexpr int capacity() noexcept { return Capacity; }

private:
    std::array<float, Capacity> dataL {}, dataR {};
    std::atomic<uint32_t> writePos { 0 };
};

} // namespace am
