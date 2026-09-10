#pragma once

#include "core/Types.h"

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace am::dev
{

/**
    Time-domain measurements of a captured audio window.

    Message thread / offline only — this is analysis for DSP LAB and the
    preset validator, never something the audio callback runs.
*/
struct SignalMetrics
{
    float peak        = 0.0f;   ///< max |sample| over both channels
    float rms         = 0.0f;   ///< RMS of the stereo pair
    float dc          = 0.0f;   ///< mean sample value (mono sum)
    float crestFactor = 0.0f;   ///< peak / rms (0 when silent)
    float correlation = 0.0f;   ///< stereo correlation, -1 .. 1 (1 for mono)
    int   nonFinite   = 0;      ///< NaN / inf samples encountered

    /** Measures a stereo window. `r` may be null for mono material. */
    static SignalMetrics measure (const float* l, const float* r, int numSamples) noexcept;
};

/**
    Reusable windowed FFT for the developer views and the sweep tool.

    Owns its scratch buffers so a view can measure every frame without
    allocating. Not real-time safe and never used from the audio thread.
*/
class SpectrumMeasurement
{
public:
    explicit SpectrumMeasurement (int fftOrder = 11);

    int fftSize() const noexcept  { return size; }
    int numBins() const noexcept  { return size / 2; }

    /** Windowed magnitude spectrum (linear, normalised by the FFT size).
        Fewer than fftSize() input samples are zero-padded. */
    void magnitudes (const float* mono, int numSamples, std::vector<float>& out);

    /** Amplitude-weighted mean frequency in Hz (0 when the window is silent). */
    float spectralCentroid (const float* mono, int numSamples, double sampleRate);

    /** Geometric mean / arithmetic mean of the magnitude spectrum, 0..1. */
    float spectralFlatness (const float* mono, int numSamples);

    /** Centroid and flatness from an already computed magnitude spectrum. */
    static float centroidOf (const std::vector<float>& mags, double sampleRate, int fftSizeUsed) noexcept;
    static float flatnessOf (const std::vector<float>& mags) noexcept;

private:
    int size;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> scratch;
    std::vector<float> mags;
};

/** One measured point of a parameter sweep (§85). */
struct SweepPoint
{
    float  normalised   = 0.0f;   ///< 0..1 position of the swept parameter
    float  value        = 0.0f;   ///< natural (denormalised) parameter value
    float  peak         = 0.0f;
    float  rms          = 0.0f;
    float  cpuPercent   = 0.0f;
    float  centroidHz   = 0.0f;
    float  crestFactor  = 0.0f;
    int    activeNodes  = 0;
    int    nonFinite    = 0;      ///< non-finite samples in the measured window
    uint32_t safetyDelta = 0;     ///< safety events raised during this step

    /** True when this step looks unsafe: clipping, non-finite audio, safety
        events or a CPU load beyond the warning threshold. */
    bool dangerous (float peakLimit = 0.99f, float cpuLimit = 80.0f) const noexcept
    {
        return peak >= peakLimit || nonFinite > 0 || safetyDelta > 0 || cpuPercent >= cpuLimit;
    }
};

/** Everything one sweep step measures. Filled by the sweep tool from the
    Master tap, the profiler and the diagnostic snapshot. */
struct SweepMeasurement
{
    const float* left      = nullptr;
    const float* right     = nullptr;
    int          numSamples = 0;
    double       sampleRate = 48000.0;
    float        normalised = 0.0f;   ///< 0..1 position of the swept parameter
    float        value      = 0.0f;   ///< natural (denormalised) parameter value
    float        cpuPercent = 0.0f;
    int          activeNodes = 0;
    uint32_t     safetyDelta = 0;     ///< safety events raised during this step
};

/** Turns one measurement window into a SweepPoint (levels, crest, non-finite
    count and the spectral centroid). Message thread / offline only. */
SweepPoint measureSweepPoint (SpectrumMeasurement& spectrum, const SweepMeasurement& in,
                              std::vector<float>& monoScratch);

/** Summary of a completed sweep. */
struct SweepSummary
{
    int   numPoints        = 0;
    int   dangerousPoints  = 0;
    float maxPeak          = 0.0f;
    float maxCpuPercent    = 0.0f;
    float minCentroidHz    = 0.0f;
    float maxCentroidHz    = 0.0f;
    int   totalNonFinite   = 0;
    uint32_t totalSafety   = 0;

    static SweepSummary of (const std::vector<SweepPoint>& points,
                            float peakLimit = 0.99f, float cpuLimit = 80.0f) noexcept;
};

} // namespace am::dev
