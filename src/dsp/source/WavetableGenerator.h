#pragma once

#include "WavetableBank.h"

#include <complex>
#include <vector>

namespace am
{

/**
    The shared, immutable table set: every bank sample, the modulator sine and
    the BLEP residual used by hard sync. Built once off the audio thread.
*/
struct WavetableCache
{
    std::vector<float> storage;                            ///< all bank samples, one allocation
    std::array<WavetableBankData, kWaveNumBanks> banks {};
    std::array<float, kWaveSineSize> sine {};
    std::vector<float> blep;                               ///< (kWaveBlepRes + 1) * kWaveBlepLen
    size_t bytes = 0;
};

/**
    Procedural wavetable designer.

    Every frame is described as a harmonic spectrum (complex bins, so frames
    blend smoothly through position and morph). Frames that are easier to
    describe in the time domain (FOLDED, FRACTURED) are rendered at 8x
    oversampling and analysed back into that spectrum. Each mip level is then
    resynthesised with an inverse FFT using only the harmonics that fit below
    Nyquist for that level, and the whole frame is scaled to a common RMS so
    scanning position never produces level jumps.
*/
class WavetableGenerator
{
public:
    static void build (WavetableCache& cache);
    static const char* bankName (int index) noexcept;

    /** Target RMS every frame is normalised to (peak-capped, see the .cpp). */
    static constexpr float kTargetRms = 0.40f;
    static constexpr float kPeakCap   = 1.30f;
};

} // namespace am
