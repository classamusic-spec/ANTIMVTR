#pragma once

#include "core/Types.h"
#include "core/Random.h"
#include "core/FastMath.h"

/**
    Excitation primitives shared by the DUST and IMPACT sources.

    Everything here is allocation-free, deterministic and safe to call from the
    audio thread. Filters expose an explicit `sanitise()` so the owning source
    can recover from a NaN/inf without dragging the whole voice down.

    The namespace `am::excitation` keeps these building blocks out of the way of
    the other DSP domains (Matter, Evolve, Space) which have their own filters.
*/
namespace am::excitation
{

//==============================================================================
/**
    Shared lookup tables for windows and excitation pulses.

    Built once on first use (from `prepare()`, never from `render()`), so the
    audio thread only ever reads them.
*/
struct Tables
{
    static constexpr int kSize = 1024;

    std::array<float, kSize + 1> hann {};   ///< raised cosine window over [0, 1]
    std::array<float, kSize + 1> sine {};   ///< sin (2 pi x) over [0, 1]
    std::array<float, kSize + 1> pulse {};  ///< one bipolar windowed cycle, peak = 1

    Tables() noexcept
    {
        float peak = 0.0f;
        for (int i = 0; i <= kSize; ++i)
        {
            const float x = (float) i / (float) kSize;
            hann[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * (float) kPi * x);
            sine[(size_t) i] = std::sin (2.0f * (float) kPi * x);
            const float p = hann[(size_t) i] * sine[(size_t) i];
            pulse[(size_t) i] = p;
            peak = std::max (peak, std::abs (p));
        }
        const float norm = peak > 1.0e-6f ? 1.0f / peak : 1.0f;
        for (auto& v : pulse) v *= norm;
    }

    static const Tables& get() noexcept
    {
        static const Tables tables;
        return tables;
    }

    /** Linear interpolation of a table for x in [0, 1). Values outside are clamped. */
    static inline float lookup (const std::array<float, kSize + 1>& t, float x) noexcept
    {
        const float pos = clamp01 (x) * (float) kSize;
        const int   i   = (int) pos;
        const float f   = pos - (float) i;
        const int   i0  = i < kSize ? i : kSize;
        const int   i1  = i0 < kSize ? i0 + 1 : kSize;
        return t[(size_t) i0] + f * (t[(size_t) i1] - t[(size_t) i0]);
    }

    static inline float hannAt  (float x) noexcept { return lookup (get().hann,  x); }
    static inline float sineAt  (float x) noexcept { return lookup (get().sine,  x); }
    static inline float pulseAt (float x) noexcept { return lookup (get().pulse, x); }
};

/** Forces the tables to exist. Call from prepare(), never from render(). */
inline void warmTables() noexcept { (void) Tables::get(); }

//==============================================================================
/** Uniform white noise in [-1, 1). RMS = 1/sqrt(3) = 0.5774. */
class WhiteNoise
{
public:
    void seed (uint32_t s) noexcept { rng.reseed (s); }
    inline float next() noexcept { return rng.nextBipolar(); }
    Rng& generator() noexcept { return rng; }

private:
    Rng rng { 1 };
};

//==============================================================================
/**
    Paul Kellet's refined pink filter: -3 dB/octave from ~10 Hz to ~20 kHz.
    Output RMS is roughly 3.4x the white input, hence `kOutputScale`.
*/
class PinkFilter
{
public:
    static constexpr float kOutputScale = 0.29f;

    void reset() noexcept { b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f; }

    inline float process (float w) noexcept
    {
        b0 = 0.99886f * b0 + w * 0.0555179f;
        b1 = 0.99332f * b1 + w * 0.0750759f;
        b2 = 0.96900f * b2 + w * 0.1538520f;
        b3 = 0.86650f * b3 + w * 0.3104856f;
        b4 = 0.55000f * b4 + w * 0.5329522f;
        b5 = -0.7616f * b5 - w * 0.0168980f;
        const float out = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f;
        b6 = w * 0.115926f;
        return out * kOutputScale;
    }

    bool sanitise() noexcept
    {
        if (isFinite (b0) && isFinite (b1) && isFinite (b2) && isFinite (b3)
            && isFinite (b4) && isFinite (b5) && isFinite (b6)) return false;
        reset();
        return true;
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
};

//==============================================================================
/**
    Leaky integrator giving a -6 dB/octave (brown / red) slope.

    The leak sets where the slope starts: a 0.008 coefficient corners at about
    61 Hz, so the -6 dB/octave law holds across the whole musical range instead
    of only above ~150 Hz. The leak still keeps the random walk bounded, and a
    DC blocker downstream removes the residual offset.
*/
class BrownFilter
{
public:
    static constexpr float kLeak = 0.008f;
    static constexpr float kOutputScale = 15.8f;

    void reset() noexcept { state = 0.0f; }

    inline float process (float w) noexcept
    {
        state += kLeak * (w - state);
        return state * kOutputScale;
    }

    bool sanitise() noexcept { if (isFinite (state)) return false; reset(); return true; }

private:
    float state = 0.0f;
};

//==============================================================================
/**
    First order difference. Applied to pink noise it produces a +3 dB/octave
    (blue) slope; applied to white noise it gives +6 dB/octave (violet).
*/
class DifferenceFilter
{
public:
    static constexpr float kOutputScale = 1.75f;

    void reset() noexcept { z1 = 0.0f; }

    inline float process (float x) noexcept
    {
        const float out = x - z1;
        z1 = x;
        return out * kOutputScale;
    }

    bool sanitise() noexcept { if (isFinite (z1)) return false; reset(); return true; }

private:
    float z1 = 0.0f;
};

//==============================================================================
/**
    One-pole tilt: boosts one half of the spectrum and cuts the other around a
    pivot frequency. `tilt` runs -1 (dark) .. +1 (bright) for +/- 9 dB, and the
    output is energy-normalised so the tilt changes colour, not loudness.
*/
class TiltFilter
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { lp = 0.0f; }

    void set (float pivotHz, float tilt) noexcept
    {
        const float w = juce::jlimit (0.0002f, 0.45f, pivotHz / (float) sr);
        a = 1.0f - std::exp (-2.0f * (float) kPi * w);
        gLow  = std::pow (2.0f, -1.5f * tilt);
        gHigh = std::pow (2.0f,  1.5f * tilt);
        norm  = 1.0f / std::sqrt (0.5f * (gLow * gLow + gHigh * gHigh));
    }

    inline float process (float x) noexcept
    {
        lp += a * (x - lp);
        return (gLow * lp + gHigh * (x - lp)) * norm;
    }

    bool sanitise() noexcept { if (isFinite (lp)) return false; reset(); return true; }

private:
    double sr = 48000.0;
    float a = 0.1f, gLow = 1.0f, gHigh = 1.0f, norm = 1.0f, lp = 0.0f;
};

//==============================================================================
/** Topology-preserving state variable filter (Zavalishin). */
class Svf
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); set (1000.0f, 0.707f); }
    void reset() noexcept { ic1 = 0.0f; ic2 = 0.0f; }

    void set (float freqHz, float q) noexcept
    {
        const float nyq = (float) (sr * 0.49);
        cutoff = juce::jlimit (5.0f, nyq, freqHz);
        resonance = juce::jlimit (0.05f, 200.0f, q);
        g  = std::tan ((float) kPi * cutoff / (float) sr);
        k  = 1.0f / resonance;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float getCutoff()    const noexcept { return cutoff; }
    float getResonance() const noexcept { return resonance; }

    inline void process (float x, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    inline float bandpass (float x) noexcept { float lp, bp, hp; process (x, lp, bp, hp); return bp; }
    inline float lowpass  (float x) noexcept { float lp, bp, hp; process (x, lp, bp, hp); return lp; }
    inline float highpass (float x) noexcept { float lp, bp, hp; process (x, lp, bp, hp); return hp; }

    /**
        Gain that keeps the band-pass output of white noise at a constant RMS
        whatever the cutoff and Q.

        This band-pass has a peak gain of Q over a -3 dB bandwidth of fc/Q, so
        white noise leaves it at RMS_in * sqrt(pi * Q * fc / sr) (verified
        against a simulation of this exact difference equation to within 8 %
        over Q = 0.7 .. 40). The reciprocal is the normalising gain.
    */
    float bandpassNoiseGain() const noexcept
    {
        const float a = std::sqrt ((float) kPi * resonance * juce::jmax (5.0f, cutoff) / (float) sr);
        return juce::jlimit (0.02f, 200.0f, 1.0f / juce::jmax (1.0e-6f, a));
    }

    /**
        The same for two of these band-passes in series (12 dB/octave skirts).
        Cascading multiplies the noise RMS by Q / sqrt(2), which the simulation
        confirms to three digits across the whole Q range.
    */
    float bandpassCascadeNoiseGain() const noexcept
    {
        return juce::jlimit (0.001f, 200.0f, bandpassNoiseGain() * 1.41421f / juce::jmax (0.05f, resonance));
    }

    /**
        The same idea for the low-pass output, whose equivalent noise bandwidth
        is pi/2 * fc * (Q + 1/(4Q)): it grows with the cutoff and with the
        resonant peak, so a noise burst keeps its loudness while brightness
        sweeps.
    */
    float lowpassNoiseGain() const noexcept
    {
        const float shape = resonance + 0.25f / juce::jmax (0.05f, resonance);
        const float a = std::sqrt ((float) kPi * juce::jmax (5.0f, cutoff) * shape / (float) sr);
        return juce::jlimit (0.05f, 60.0f, 1.0f / juce::jmax (1.0e-6f, a));
    }

    bool sanitise() noexcept
    {
        if (isFinite (ic1) && isFinite (ic2)) return false;
        reset();
        return true;
    }

private:
    double sr = 48000.0;
    float cutoff = 1000.0f, resonance = 0.707f;
    float g = 0.1f, k = 1.4f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
};

//==============================================================================
/** One-pole DC blocker (high pass). */
class DcBlocker
{
public:
    void prepare (double sampleRate, float cornerHz = 12.0f) noexcept
    {
        r = std::exp (-2.0f * (float) kPi * cornerHz / (float) sampleRate);
        reset();
    }

    void reset() noexcept { x1 = 0.0f; y1 = 0.0f; }

    inline float process (float x) noexcept
    {
        const float y = x - x1 + r * y1;
        x1 = x;
        y1 = y;
        return y;
    }

    bool sanitise() noexcept { if (isFinite (x1) && isFinite (y1)) return false; reset(); return true; }

private:
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

//==============================================================================
/**
    Two cascaded one-poles used to turn a unit sample impulse into a
    band-limited excitation pulse with a flat spectrum from DC to the cutoff.

    `impulseGain()` is the analytic scale that makes the impulse response carry
    unit energy, so a spike train keeps the same loudness at any cutoff:

        h[n] = a^2 (n+1) b^n,  b = 1 - a
        E    = sum h[n]^2 = a^4 (1 + b^2) / (1 - b^2)^3
*/
class ExciterLowpass
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); setCutoff (5000.0f); }
    void reset() noexcept { z1 = 0.0f; z2 = 0.0f; }

    void setCutoff (float hz) noexcept
    {
        const float nyq = (float) (sr * 0.48);
        cutoff = juce::jlimit (20.0f, nyq, hz);
        const float w = 2.0f * (float) kPi * cutoff / (float) sr;
        a = 1.0f - std::exp (-w);
        a = juce::jlimit (1.0e-4f, 1.0f, a);
        const float b  = 1.0f - a;
        const float b2 = b * b;
        const float den = juce::jmax (1.0e-9f, 1.0f - b2);
        const float energy = a * a * a * a * (1.0f + b2) / (den * den * den);
        gain = 1.0f / std::sqrt (juce::jmax (1.0e-12f, energy));

        // Peak of h[n] = a^2 (n+1) b^n, which is reached at n ~ -1/ln(b) - 1.
        float peak = a * a;
        if (b > 1.0e-6f)
        {
            const float nStar = juce::jmax (0.0f, -1.0f / std::log (b) - 1.0f);
            const float n0 = std::floor (nStar);
            const float h0 = a * a * (n0 + 1.0f) * std::pow (b, n0);
            const float h1 = a * a * (n0 + 2.0f) * std::pow (b, n0 + 1.0f);
            peak = juce::jmax (peak, h0, h1);
        }
        peakGain = 1.0f / juce::jmax (1.0e-9f, peak);
    }

    float getCutoff() const noexcept { return cutoff; }

    /** Multiply a unit spike by this so the filtered pulse carries unit energy.
        The same factor normalises the RMS of white noise pushed through the filter. */
    float impulseGain() const noexcept { return gain; }

    /** Multiply a unit spike by this so the filtered pulse has a peak of exactly 1. */
    float impulsePeakGain() const noexcept { return peakGain; }

    inline float process (float x) noexcept
    {
        z1 += a * (x - z1);
        z2 += a * (z1 - z2);
        return z2;
    }

    bool sanitise() noexcept { if (isFinite (z1) && isFinite (z2)) return false; reset(); return true; }

private:
    double sr = 48000.0;
    float cutoff = 5000.0f, a = 0.5f, gain = 1.0f, peakGain = 1.0f, z1 = 0.0f, z2 = 0.0f;
};

//==============================================================================
/**
    Transparent below 0.9, hard bounded at 1.0. Used as the final guard of every
    source so a dense roll of strikes or a stack of grains can never leave the
    legal range, without colouring normal levels.
*/
inline float softLimit (float x) noexcept
{
    constexpr float knee = 0.9f;
    const float mag = std::abs (x);
    if (mag <= knee) return x;
    const float over = (mag - knee) / (1.0f - knee);
    const float shaped = knee + (1.0f - knee) * fastTanh (over);
    return x < 0.0f ? -shaped : shaped;
}

/** Equal-power pan. `pan` runs -1 (left) .. +1 (right). */
inline void panGains (float pan, float& lGain, float& rGain) noexcept
{
    const float p = juce::jlimit (-1.0f, 1.0f, pan) * 0.5f + 0.5f;
    lGain = Tables::sineAt (0.25f - p * 0.25f);   // cos (p * pi/2)
    rGain = Tables::sineAt (p * 0.25f);           // sin (p * pi/2)
}

/** Exponential mapping helper: returns lo at t = 0 and hi at t = 1. */
inline float expMap (float t, float lo, float hi) noexcept
{
    return lo * std::pow (hi / lo, clamp01 (t));
}

} // namespace am::excitation
