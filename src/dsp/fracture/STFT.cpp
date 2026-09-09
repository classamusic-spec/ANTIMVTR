#include "STFT.h"

namespace am
{

void STFT::prepare (int fftSize)
{
    fftSizeValue = juce::nextPowerOfTwo (juce::jlimit (64, 8192, fftSize));
    hopSizeValue = fftSizeValue / 4;

    const int order = (int) std::round (std::log2 ((double) fftSizeValue));
    fft = std::make_unique<juce::dsp::FFT> (order);

    window.resize ((size_t) fftSizeValue);
    double sumSquares = 0.0;
    for (int i = 0; i < fftSizeValue; ++i)
    {
        // Periodic Hann — sums to a constant under 75 % overlap-add.
        const double w = 0.5 - 0.5 * std::cos (kTwoPi * (double) i / (double) fftSizeValue);
        window[(size_t) i] = (float) w;
        sumSquares += w * w;
    }

    // Σ_k w²[n - k·hop] over the overlapping frames = (Σ_n w²[n]) / hop
    // (= 1.5 for a periodic Hann at 75 % overlap).
    const double colaSum = sumSquares / (double) hopSizeValue;
    synthScale = (float) (1.0 / juce::jmax (1.0e-9, colaSum));

    for (int ch = 0; ch < 2; ++ch)
    {
        inFifo[ch].assign ((size_t) fftSizeValue, 0.0f);
        outFifo[ch].assign ((size_t) hopSizeValue, 0.0f);
        outAccum[ch].assign ((size_t) fftSizeValue, 0.0f);
        fftBuf[ch].assign ((size_t) (2 * fftSizeValue), 0.0f);
    }

    reset();
}

void STFT::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        std::fill (inFifo[ch].begin(), inFifo[ch].end(), 0.0f);
        std::fill (outFifo[ch].begin(), outFifo[ch].end(), 0.0f);
        std::fill (outAccum[ch].begin(), outAccum[ch].end(), 0.0f);
        std::fill (fftBuf[ch].begin(), fftBuf[ch].end(), 0.0f);
    }
    rover = fftSizeValue - hopSizeValue;
}

} // namespace am
