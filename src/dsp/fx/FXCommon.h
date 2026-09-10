#pragma once

#include "dsp/RenderContext.h"
#include "core/FastMath.h"
#include "core/Random.h"
#include "core/RealtimeUtils.h"
#include "core/Smoothing.h"

#include <vector>

/**
    Shared real-time primitives for the SPACE FX rack.

    Everything here is allocation free after `prepare()`: buffers are sized
    once from the sample rate, every recursive structure is guarded against
    denormals and non-finite values, and every control value is smoothed so
    modules can be automated without zipper noise.
*/
namespace am::fx
{

inline constexpr float kTinyGuard = 1.0e-20f;

inline float clampf (float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/** Guards a recursive state variable: kills NaN/inf and denormals in one step. */
inline float sanitise (float v) noexcept
{
    if (! std::isfinite (v)) return 0.0f;
    return std::abs (v) < 1.0e-25f ? 0.0f : v;
}

/** Equal-power dry/wet pair for a 0..1 mix control. */
inline void equalPower (float mix, float& dryGain, float& wetGain) noexcept
{
    mix = clampf (mix, 0.0f, 1.0f);
    if (mix <= 0.0f)      { dryGain = 1.0f; wetGain = 0.0f; return; }
    if (mix >= 1.0f)      { dryGain = 0.0f; wetGain = 1.0f; return; }
    const float a = mix * (float) kPi * 0.5f;
    dryGain = std::cos (a);
    wetGain = std::sin (a);
}

/** Exponential map of a 0..1 control onto [lo, hi]. */
inline float expMap (float t, float lo, float hi) noexcept
{
    return lo * std::pow (hi / lo, clampf (t, 0.0f, 1.0f));
}

//==============================================================================
/** A control value with per-sample one-pole smoothing (default 12 ms). */
class SmoothParam
{
public:
    void prepare (double sampleRate, float timeMs = 12.0f) noexcept
    {
        smoother.prepare (sampleRate, timeMs);
        smoother.reset (smoother.getTarget());
    }

    void reset (float v) noexcept       { smoother.reset (v); }
    void setTarget (float v) noexcept   { smoother.setTarget (v); }
    inline float next() noexcept        { return smoother.next(); }
    float current() const noexcept      { return smoother.getCurrent(); }
    float target() const noexcept       { return smoother.getTarget(); }
    void skip (int n) noexcept          { smoother.skip (n); }
    bool moving() const noexcept        { return smoother.isSmoothing(); }

private:
    OnePoleSmoother smoother;
};

//==============================================================================
/** Topology-preserving one-pole filter (stable at every cutoff, cheap). */
struct OnePoleTPT
{
    void prepare (double sampleRate) noexcept { sr = sampleRate; setCutoff (1000.0f); reset(); }
    void reset() noexcept { z = 0.0f; }

    void setCutoff (float hz) noexcept
    {
        hz = clampf (hz, 4.0f, (float) (sr * 0.49));
        const float g = std::tan ((float) kPi * hz / (float) sr);
        G = g / (1.0f + g);
    }

    inline float lp (float x) noexcept
    {
        const float v = (x - z) * G;
        const float y = v + z;
        z = sanitise (y + v);
        return y;
    }

    inline float hp (float x) noexcept { return x - lp (x); }

    double sr = 48000.0;
    float G = 0.1f, z = 0.0f;
};

//==============================================================================
/** RBJ biquad, transposed direct form II. */
struct Biquad
{
    void reset() noexcept { z1 = z2 = 0.0f; }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = sanitise (b1 * x - a1 * y + z2);
        z2 = sanitise (b2 * x - a2 * y);
        return y;
    }

    void setIdentity() noexcept { b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f; }

    void setLowShelf (double sr, float freq, float q, float gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = kTwoPi * clampf (freq, 20.0f, (float) (sr * 0.45)) / sr;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * std::max (0.1, (double) q));
        const double sq = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1.0) + (A - 1.0) * cw + sq;
        b0 = (float) (A * ((A + 1.0) - (A - 1.0) * cw + sq) / a0);
        b1 = (float) (2.0 * A * ((A - 1.0) - (A + 1.0) * cw) / a0);
        b2 = (float) (A * ((A + 1.0) - (A - 1.0) * cw - sq) / a0);
        a1 = (float) (-2.0 * ((A - 1.0) + (A + 1.0) * cw) / a0);
        a2 = (float) (((A + 1.0) + (A - 1.0) * cw - sq) / a0);
    }

    void setHighShelf (double sr, float freq, float q, float gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = kTwoPi * clampf (freq, 20.0f, (float) (sr * 0.45)) / sr;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * std::max (0.1, (double) q));
        const double sq = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1.0) - (A - 1.0) * cw + sq;
        b0 = (float) (A * ((A + 1.0) + (A - 1.0) * cw + sq) / a0);
        b1 = (float) (-2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0);
        b2 = (float) (A * ((A + 1.0) + (A - 1.0) * cw - sq) / a0);
        a1 = (float) (2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0);
        a2 = (float) (((A + 1.0) - (A - 1.0) * cw - sq) / a0);
    }

    void setPeak (double sr, float freq, float q, float gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = kTwoPi * clampf (freq, 20.0f, (float) (sr * 0.45)) / sr;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * std::max (0.1, (double) q));
        const double a0 = 1.0 + alpha / A;
        b0 = (float) ((1.0 + alpha * A) / a0);
        b1 = (float) (-2.0 * cw / a0);
        b2 = (float) ((1.0 - alpha * A) / a0);
        a1 = (float) (-2.0 * cw / a0);
        a2 = (float) ((1.0 - alpha / A) / a0);
    }

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

//==============================================================================
/**
    Fractional delay line with a power-of-two buffer. Linear reads for static
    taps, 4-point Hermite for modulated ones (no zipper on moving heads).
*/
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int n = 8;
        while (n < maxDelaySamples + 8) n <<= 1;
        size = n;
        mask = n - 1;
        buffer.assign ((size_t) n, 0.0f);
        writePos = 0;
    }

    void clear() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    inline void write (float x) noexcept
    {
        buffer[(size_t) writePos] = sanitise (x);
        writePos = (writePos + 1) & mask;
    }

    inline float readLinear (float delaySamples) const noexcept
    {
        const float d = clampf (delaySamples, 1.0f, (float) (size - 4));
        float rp = (float) writePos - d;
        if (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const float frac = rp - (float) i0;
        const float a = buffer[(size_t) (i0 & mask)];
        const float b = buffer[(size_t) ((i0 + 1) & mask)];
        return a + frac * (b - a);
    }

    inline float readHermite (float delaySamples) const noexcept
    {
        const float d = clampf (delaySamples, 2.0f, (float) (size - 4));
        float rp = (float) writePos - d;
        if (rp < 0.0f) rp += (float) size;
        const int i1 = (int) rp;
        const float t = rp - (float) i1;
        const float xm1 = buffer[(size_t) ((i1 - 1) & mask)];
        const float x0  = buffer[(size_t) (i1 & mask)];
        const float x1  = buffer[(size_t) ((i1 + 1) & mask)];
        const float x2  = buffer[(size_t) ((i1 + 2) & mask)];
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    /** Reads at an absolute (already wrapped) position — used by granular heads. */
    inline float readAt (double position) const noexcept
    {
        const double wrapped = position - std::floor (position / (double) size) * (double) size;
        const int i0 = (int) wrapped;
        const float frac = (float) (wrapped - (double) i0);
        const float a = buffer[(size_t) (i0 & mask)];
        const float b = buffer[(size_t) ((i0 + 1) & mask)];
        return a + frac * (b - a);
    }

    int writeIndex() const noexcept { return writePos; }
    int capacity() const noexcept { return size; }

private:
    std::vector<float> buffer;
    int size = 8, mask = 7, writePos = 0;
};

//==============================================================================
/** Schroeder allpass (fixed delay), the building block of every diffuser. */
class Allpass
{
public:
    void prepare (int maxDelaySamples) { line.prepare (maxDelaySamples); }
    void clear() noexcept { line.clear(); }

    void set (float delaySamples, float coefficient) noexcept
    {
        delay = clampf (delaySamples, 2.0f, (float) (line.capacity() - 4));
        g = clampf (coefficient, -0.85f, 0.85f);
    }

    inline float process (float x) noexcept
    {
        const float d = line.readLinear (delay);
        const float v = x - g * d;
        line.write (v);
        return d + g * v;
    }

    /** Modulated variant: `offset` is added to the delay (Hermite read). */
    inline float processModulated (float x, float offset) noexcept
    {
        const float d = line.readHermite (delay + offset);
        const float v = x - g * d;
        line.write (v);
        return d + g * v;
    }

private:
    DelayLine line;
    float delay = 32.0f, g = 0.5f;
};

//==============================================================================
/** Cheap sine LFO (phase accumulator + parabolic approximation). */
struct LFO
{
    void prepare (double sampleRate) noexcept { sr = sampleRate; }
    void reset (float startPhase = 0.0f) noexcept { phase = startPhase - std::floor (startPhase); }
    void setRate (float hz) noexcept { inc = (float) (clampf (hz, 0.0f, 40.0f) / sr); }

    inline float next() noexcept
    {
        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;
        return fastSin01 (phase);
    }

    double sr = 48000.0;
    float phase = 0.0f, inc = 0.0f;
};

//==============================================================================
/**
    Click-free enable gate. Modules are crossfaded in and out over a few
    milliseconds; when the gate is fully closed the module can be skipped
    (and, after a moment, reset to release its tail).
*/
class ModuleGate
{
public:
    void prepare (double sampleRate, float fadeMs = 8.0f) noexcept
    {
        step = 1.0f / std::max (1.0f, (float) (sampleRate * (double) fadeMs * 0.001));
        gain = target = 0.0f;
    }

    void setEnabled (bool shouldBeOn) noexcept { target = shouldBeOn ? 1.0f : 0.0f; }
    void snapTo (bool on) noexcept { target = gain = on ? 1.0f : 0.0f; }
    /** Jumps to the pending state — only valid when nothing is playing yet. */
    void snapToTarget() noexcept { gain = target; }

    inline float next() noexcept
    {
        if (gain < target)      gain = std::min (target, gain + step);
        else if (gain > target) gain = std::max (target, gain - step);
        return gain;
    }

    /** True when the module has to run this block (audible or still fading). */
    bool active() const noexcept { return gain > 0.0f || target > 0.0f; }
    bool closed() const noexcept { return gain <= 0.0f && target <= 0.0f; }
    /** Fully open with nothing pending: the crossfade can be skipped. */
    bool settledOpen() const noexcept { return gain >= 1.0f && target >= 1.0f; }
    float value() const noexcept { return gain; }

private:
    float gain = 0.0f, target = 0.0f, step = 0.01f;
};

//==============================================================================
/** Orthogonal 8x8 Hadamard mixing (fast in-place butterflies, unity gain). */
inline void hadamard8 (float* v) noexcept
{
    for (int step = 1; step < 8; step <<= 1)
        for (int i = 0; i < 8; i += step * 2)
            for (int j = i; j < i + step; ++j)
            {
                const float a = v[j], b = v[j + step];
                v[j] = a + b;
                v[j + step] = a - b;
            }

    constexpr float norm = 0.35355339059327379f;   // 1 / sqrt(8)
    for (int i = 0; i < 8; ++i) v[i] *= norm;
}

//==============================================================================
/**
    Tilt filter used by the TONE macro: one pivot frequency, low and high
    shelves moving in opposite directions. tilt < 0 = dark, > 0 = bright.
*/
class TiltFilter
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        setTilt (0.0f);
        reset();
    }

    void reset() noexcept { low[0].reset(); low[1].reset(); high[0].reset(); high[1].reset(); }

    /** tilt in -1..1, maxDb the shelf gain at the extremes. */
    void setTilt (float tilt, float maxDb = 7.0f) noexcept
    {
        tilt = clampf (tilt, -1.0f, 1.0f);
        if (std::abs (tilt - currentTilt) < 1.0e-4f) return;
        currentTilt = tilt;
        const float db = tilt * maxDb;
        for (int c = 0; c < 2; ++c)
        {
            low[c].setLowShelf (sr, 520.0f, 0.5f, -db);
            high[c].setHighShelf (sr, 2200.0f, 0.5f, db);
        }
    }

    inline float process (int channel, float x) noexcept
    {
        return high[channel].process (low[channel].process (x));
    }

private:
    double sr = 48000.0;
    float currentTilt = 999.0f;
    Biquad low[2], high[2];
};

} // namespace am::fx
