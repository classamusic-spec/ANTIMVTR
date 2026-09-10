#pragma once

#include "core/Types.h"
#include "core/RealtimeHandoff.h"

#include <memory>
#include <vector>

namespace am
{

/**
    An immutable block of audio shared by every voice.

    `SampleData` objects are built on the message thread (built-in generators
    or a decoded user file) and handed to the audio thread through a
    `RealtimeHandoff`; the audio thread only ever reads them and never frees
    them. Nothing here is called from `render()` except the accessors.

    The channels carry `kPad` zero samples before and after the audio so a
    four point interpolator can read `pos - 1 .. pos + 2` around any legal
    position without a per-sample branch; readers still clamp the index, so a
    corrupt position can never leave the allocation.
*/
struct SampleData
{
    static constexpr int kPad = 4;

    SampleData() noexcept { liveInstances.fetch_add (1, std::memory_order_relaxed); }
    ~SampleData() noexcept
    {
        liveInstances.fetch_sub (1, std::memory_order_relaxed);
        lastFreeThread.store ((uint64_t) (uintptr_t) juce::Thread::getCurrentThreadId(), std::memory_order_relaxed);
    }

    SampleData (const SampleData&) = delete;
    SampleData& operator= (const SampleData&) = delete;

    juce::String name;          ///< display name ("Glass Strike", or the file name)
    juce::String path;          ///< user file path, empty for a built-in
    int    builtInIndex = -1;   ///< index into BuiltInSamples, -1 for a user file
    double sampleRate   = 48000.0;
    int    numChannels  = 1;
    int    numFrames    = 0;
    float  peak         = 0.0f;

    /** Channel storage including the leading/trailing pad. Use `read()` / `channel()`. */
    std::vector<float> data[2];

    /** Allocates (message thread only) `frames` samples on `channels` channels, zeroed. */
    void allocate (int channels, int frames)
    {
        numChannels = juce::jlimit (1, 2, channels);
        numFrames   = juce::jmax (0, frames);
        for (int c = 0; c < 2; ++c)
        {
            data[c].assign ((size_t) (c < numChannels ? numFrames + 2 * kPad : 0), 0.0f);
        }
    }

    /** Writable pointer to the first audio sample of a channel (message thread only). */
    float* write (int channel) noexcept
    {
        const int c = juce::jlimit (0, numChannels - 1, channel);
        return data[c].empty() ? nullptr : data[c].data() + kPad;
    }

    /** First audio sample of a channel; mono samples return channel 0 for both. */
    const float* channel (int c) const noexcept
    {
        const int ch = numChannels > 1 ? juce::jlimit (0, 1, c) : 0;
        return data[ch].empty() ? nullptr : data[ch].data() + kPad;
    }

    bool isEmpty() const noexcept { return numFrames <= 0 || data[0].empty(); }
    double lengthSeconds() const noexcept { return sampleRate > 0.0 ? (double) numFrames / sampleRate : 0.0; }

    /** Recomputes `peak` after filling the channels. */
    void updatePeak() noexcept
    {
        peak = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            if (const float* s = channel (c))
                for (int i = 0; i < numFrames; ++i)
                    peak = juce::jmax (peak, std::abs (s[i]));
    }

    /**
        Four point cubic Hermite (Catmull-Rom) read at a fractional frame.
        The index is clamped into the padded allocation, so any position —
        including NaN, which clamps to 0 — is safe.
    */
    inline float read (int ch, double position) const noexcept
    {
        const float* s = channel (ch);
        if (s == nullptr || numFrames <= 0) return 0.0f;

        double base = std::floor (position);
        if (! (base >= -1.0)) base = -1.0;                       // also catches NaN
        if (base > (double) (numFrames)) base = (double) numFrames;
        const int   i = (int) base;
        float f = (float) (position - base);
        if (! (f >= 0.0f)) f = 0.0f;    // also catches NaN
        if (f > 1.0f) f = 1.0f;

        const float y0 = s[i - 1], y1 = s[i], y2 = s[i + 1], y3 = s[i + 2];
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }

    /** Live `SampleData` objects (diagnostics: proves nothing leaks and nothing is freed too early). */
    static inline std::atomic<int> liveInstances { 0 };
    /** Thread id of the most recent destruction (tests assert this is never the audio thread). */
    static inline std::atomic<uint64_t> lastFreeThread { 0 };
};

/** Shared, immutable reference. The audio thread reads the pointee, never the count. */
using SampleRef = std::shared_ptr<const SampleData>;

/** Message thread → audio thread transport for the current sample. */
using SampleHandoff = RealtimeHandoff<SampleRef>;

//==============================================================================
/**
    The built-in sample set — synthesised from modal, noise and impulse
    recipes so SAMPLE works out of the box and the product ships no binary
    assets. Generation happens on the message thread (startup / patch load).
*/
namespace BuiltInSamples
{
    enum class Kind : int
    {
        GlassStrike = 0, MetalPing, WoodKnock, StoneDrop, Breath, VinylDust, NoiseBurst, Count
    };

    int count() noexcept;
    const char* name (int index) noexcept;

    /** Index of a built-in by name, or -1. */
    int indexOf (const juce::String& name) noexcept;

    /** Builds one built-in sample (message thread). `index` is clamped. */
    std::shared_ptr<const SampleData> create (int index, double sampleRate = 48000.0);
}

//==============================================================================
/**
    Decodes an audio file into a `SampleData` (message thread only — this
    touches the disk). Returns null and fills `error` when the file cannot be
    read; callers fall back to a built-in instead of failing.

    Longer files are truncated to `maxSeconds` and more than two channels are
    mixed down to stereo, which bounds what a patch can pull into memory.
*/
std::shared_ptr<const SampleData> loadSampleFile (const juce::File& file, juce::String* error = nullptr,
                                                  double maxSeconds = 30.0);

} // namespace am
