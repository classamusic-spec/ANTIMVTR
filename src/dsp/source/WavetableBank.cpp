#include "WavetableBank.h"
#include "WavetableGenerator.h"

namespace am
{

namespace
{
    /**
        The one shared table set. Built lazily on the first prepare() (message
        thread) and immutable afterwards, so every voice of every instance
        reads the same 3.6 MiB without any per-voice allocation.
    */
    const WavetableCache& sharedCache()
    {
        static const WavetableCache instance = []
        {
            WavetableCache c;
            WavetableGenerator::build (c);
            return c;
        }();
        return instance;
    }
}

void Wavetables::prewarm()
{
    sharedCache();
}

const WavetableBankData& Wavetables::bank (int index) noexcept
{
    return sharedCache().banks[(size_t) juce::jlimit (0, kWaveNumBanks - 1, index)];
}

const char* Wavetables::bankName (int index) noexcept
{
    return WavetableGenerator::bankName (index);
}

size_t Wavetables::memoryBytes() noexcept
{
    return sharedCache().bytes;
}

const float* Wavetables::sineTable() noexcept
{
    return sharedCache().sine.data();
}

const float* Wavetables::blepTable() noexcept
{
    return sharedCache().blep.data();
}

float Wavetables::framePosition (int bankIndex, float position) noexcept
{
    const auto& b = bank (bankIndex);
    return clamp01 (position) * (float) (b.numFrames - 1);
}

void Wavetables::renderDisplayFrame (int bankIndex, float position, float morph,
                                     float* out, int numSamples) noexcept
{
    if (out == nullptr || numSamples <= 0) return;

    const auto& b = bank (bankIndex);
    const auto& level = b.levels[0];

    const float fpos = framePosition (bankIndex, position);
    const int   frameA = juce::jlimit (0, b.numFrames - 2, (int) fpos);
    const float blend  = juce::jlimit (0.0f, 1.0f, fpos - (float) frameA);
    const float pivot  = waveMorphPivot (morph);

    const float* fa = level.frame (frameA);
    const float* fb = level.frame (frameA + 1);

    const float step = 1.0f / (float) numSamples;
    for (int i = 0; i < numSamples; ++i)
    {
        const float phase = waveMorphWarp ((float) i * step, pivot);
        out[i] = waveReadFrames (fa, fb, blend, level.mask, level.lengthF, phase);
    }
}

} // namespace am
