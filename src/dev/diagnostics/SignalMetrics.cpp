#include "SignalMetrics.h"

#include <algorithm>
#include <cmath>

namespace am::dev
{

SignalMetrics SignalMetrics::measure (const float* l, const float* r, int numSamples) noexcept
{
    SignalMetrics m;
    if (l == nullptr || numSamples <= 0)
        return m;

    double sumSquares = 0.0, sumMono = 0.0;
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
    float peak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float a = l[i];
        float b = r != nullptr ? r[i] : l[i];

        if (! std::isfinite (a)) { ++m.nonFinite; a = 0.0f; }
        if (r != nullptr && ! std::isfinite (b)) { ++m.nonFinite; b = 0.0f; }

        peak = std::max (peak, std::max (std::abs (a), std::abs (b)));
        sumSquares += 0.5 * ((double) a * a + (double) b * b);
        sumMono    += 0.5 * ((double) a + (double) b);
        sumLL      += (double) a * a;
        sumRR      += (double) b * b;
        sumLR      += (double) a * b;
    }

    const double n = (double) numSamples;
    m.peak = peak;
    m.rms  = (float) std::sqrt (sumSquares / n);
    m.dc   = (float) (sumMono / n);

    m.crestFactor = m.rms > 1.0e-9f ? m.peak / m.rms : 0.0f;

    const double denom = std::sqrt (sumLL * sumRR);
    m.correlation = denom > 1.0e-18 ? (float) juce::jlimit (-1.0, 1.0, sumLR / denom) : 0.0f;
    return m;
}

//==============================================================================
SpectrumMeasurement::SpectrumMeasurement (int fftOrder)
    : size (1 << juce::jlimit (5, 14, fftOrder)),
      fft (juce::jlimit (5, 14, fftOrder)),
      window ((size_t) size, juce::dsp::WindowingFunction<float>::hann)
{
    scratch.assign ((size_t) size * 2, 0.0f);
    mags.assign ((size_t) (size / 2), 0.0f);
}

void SpectrumMeasurement::magnitudes (const float* mono, int numSamples, std::vector<float>& out)
{
    out.assign ((size_t) (size / 2), 0.0f);
    if (mono == nullptr || numSamples <= 0)
        return;

    std::fill (scratch.begin(), scratch.end(), 0.0f);
    const int n = std::min (numSamples, size);
    // Use the most recent `n` samples of the window.
    const float* src = mono + (numSamples - n);
    for (int i = 0; i < n; ++i)
        scratch[(size_t) i] = std::isfinite (src[i]) ? src[i] : 0.0f;

    window.multiplyWithWindowingTable (scratch.data(), (size_t) size);
    fft.performFrequencyOnlyForwardTransform (scratch.data(), true);

    const float norm = 2.0f / (float) size;
    for (int i = 0; i < size / 2; ++i)
        out[(size_t) i] = scratch[(size_t) i] * norm;
}

float SpectrumMeasurement::centroidOf (const std::vector<float>& m, double sampleRate, int fftSizeUsed) noexcept
{
    if (m.empty() || fftSizeUsed <= 0 || sampleRate <= 0.0)
        return 0.0f;

    double weighted = 0.0, total = 0.0;
    const double binHz = sampleRate / (double) fftSizeUsed;
    for (size_t i = 1; i < m.size(); ++i)
    {
        const double a = (double) m[i];
        weighted += a * ((double) i * binHz);
        total    += a;
    }
    return total > 1.0e-12 ? (float) (weighted / total) : 0.0f;
}

float SpectrumMeasurement::flatnessOf (const std::vector<float>& m) noexcept
{
    if (m.size() < 2)
        return 0.0f;

    double logSum = 0.0, sum = 0.0;
    int count = 0;
    for (size_t i = 1; i < m.size(); ++i)
    {
        const double a = (double) m[i] + 1.0e-12;
        logSum += std::log (a);
        sum += a;
        ++count;
    }
    if (count == 0 || sum <= 0.0)
        return 0.0f;

    const double geometric = std::exp (logSum / (double) count);
    const double arithmetic = sum / (double) count;
    return (float) juce::jlimit (0.0, 1.0, geometric / arithmetic);
}

float SpectrumMeasurement::spectralCentroid (const float* mono, int numSamples, double sampleRate)
{
    magnitudes (mono, numSamples, mags);
    return centroidOf (mags, sampleRate, size);
}

float SpectrumMeasurement::spectralFlatness (const float* mono, int numSamples)
{
    magnitudes (mono, numSamples, mags);
    return flatnessOf (mags);
}

//==============================================================================
SweepPoint measureSweepPoint (SpectrumMeasurement& spectrum, const SweepMeasurement& in,
                              std::vector<float>& monoScratch)
{
    SweepPoint p;
    p.normalised = in.normalised;
    p.value = in.value;
    p.cpuPercent = std::isfinite (in.cpuPercent) ? in.cpuPercent : 0.0f;
    p.activeNodes = in.activeNodes;
    p.safetyDelta = in.safetyDelta;

    if (in.left == nullptr || in.numSamples <= 0)
        return p;

    const auto metrics = SignalMetrics::measure (in.left, in.right, in.numSamples);
    p.peak = metrics.peak;
    p.rms = metrics.rms;
    p.crestFactor = metrics.crestFactor;
    p.nonFinite = metrics.nonFinite;

    monoScratch.assign ((size_t) in.numSamples, 0.0f);
    for (int i = 0; i < in.numSamples; ++i)
    {
        const float l = in.left[i];
        const float r = in.right != nullptr ? in.right[i] : l;
        monoScratch[(size_t) i] = 0.5f * ((std::isfinite (l) ? l : 0.0f) + (std::isfinite (r) ? r : 0.0f));
    }
    p.centroidHz = spectrum.spectralCentroid (monoScratch.data(), in.numSamples, in.sampleRate);
    return p;
}

SweepSummary SweepSummary::of (const std::vector<SweepPoint>& points, float peakLimit, float cpuLimit) noexcept
{
    SweepSummary s;
    s.numPoints = (int) points.size();
    if (points.empty())
        return s;

    s.minCentroidHz = points.front().centroidHz;
    s.maxCentroidHz = points.front().centroidHz;

    for (const auto& p : points)
    {
        s.maxPeak = std::max (s.maxPeak, p.peak);
        s.maxCpuPercent = std::max (s.maxCpuPercent, p.cpuPercent);
        s.minCentroidHz = std::min (s.minCentroidHz, p.centroidHz);
        s.maxCentroidHz = std::max (s.maxCentroidHz, p.centroidHz);
        s.totalNonFinite += p.nonFinite;
        s.totalSafety += p.safetyDelta;
        if (p.dangerous (peakLimit, cpuLimit))
            ++s.dangerousPoints;
    }
    return s;
}

} // namespace am::dev
