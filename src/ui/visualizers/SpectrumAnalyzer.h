#pragma once

#include <juce_dsp/juce_dsp.h>
#include "core/RealtimeUtils.h"

namespace am::ui
{

/** Message-thread FFT helper that turns tap samples into log-spaced band magnitudes (0..1). */
class SpectrumAnalyzer
{
public:
    static constexpr int kOrder = 11;              // 2048-point FFT
    static constexpr int kSize  = 1 << kOrder;

    SpectrumAnalyzer() : fft (kOrder), window (kSize, juce::dsp::WindowingFunction<float>::hann) {}

    /** Computes `numBands` log-spaced magnitudes from a mono buffer of at least kSize samples. */
    void compute (const float* samples, double sampleRate, float* bandsOut, int numBands, float floorDb = -84.0f)
    {
        std::copy (samples, samples + kSize, data.begin());
        std::fill (data.begin() + kSize, data.end(), 0.0f);
        window.multiplyWithWindowingTable (data.data(), kSize);
        fft.performFrequencyOnlyForwardTransform (data.data(), true);

        const double minHz = 30.0, maxHz = juce::jmin (18000.0, sampleRate * 0.45);
        for (int b = 0; b < numBands; ++b)
        {
            const double f0 = minHz * std::pow (maxHz / minHz, (double) b / numBands);
            const double f1 = minHz * std::pow (maxHz / minHz, (double) (b + 1) / numBands);
            int i0 = (int) (f0 / sampleRate * kSize), i1 = (int) (f1 / sampleRate * kSize);
            i0 = juce::jlimit (1, kSize / 2 - 1, i0); i1 = juce::jlimit (i0 + 1, kSize / 2, i1);
            float peak = 0.0f;
            for (int i = i0; i < i1; ++i) peak = juce::jmax (peak, data[(size_t) i]);
            const float db = juce::Decibels::gainToDecibels (peak / (float) kSize * 4.0f, floorDb);
            bandsOut[b] = juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb);
        }
    }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, kSize * 2> data {};
};

} // namespace am::ui
