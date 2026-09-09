#pragma once

#include "core/Types.h"

/**
    WAVETABLE STORAGE — the immutable, shared, band-limited table set used by
    the WAVE source.

    Geometry
      * 8 banks (BASIC, HARMONIC, FORMANT, FOLDED, METALLIC, SPECTRAL,
        FRACTURED, NOISE), 16 frames each.
      * Every frame is stored as a mip pyramid: level L contains only the
        harmonics that stay below Nyquist when the oscillator increment
        reaches the top of that level's range, so playback never aliases.
      * Level lengths shrink with the harmonic count (4x oversampled relative
        to the highest harmonic, clamped to [256, 2048]) which keeps the whole
        set around 3.6 MiB while leaving cubic interpolation plenty of room.

    The tables are pure functions of the design code: they do not depend on
    the sample rate (the mip level is chosen from the normalised increment),
    so a single lazily built cache serves every voice and every instance.
*/
namespace am
{

//==============================================================================
constexpr int kWaveNumBanks      = 8;
constexpr int kWaveFramesPerBank = 16;
constexpr int kWaveNumLevels     = 11;    ///< level 0 = 1024 harmonics … level 10 = 1
constexpr int kWaveMaxHarmonics  = 1024;
constexpr int kWaveMaxLength     = 2048;
constexpr int kWaveMinLength     = 256;

/** Increment at which mip level 0 runs out of harmonic headroom (0.5 / 1024). */
constexpr float kWaveIncRef = 0.5f / (float) kWaveMaxHarmonics;

/** Samples stored per frame at a given mip level. */
constexpr int waveLevelLength (int level) noexcept
{
    const int nominal = kWaveMaxHarmonics >> level;
    const int len = 4 * nominal;
    return len > kWaveMaxLength ? kWaveMaxLength : (len < kWaveMinLength ? kWaveMinLength : len);
}

/** Highest harmonic present at a given mip level. */
constexpr int waveLevelHarmonics (int level) noexcept
{
    const int nominal = kWaveMaxHarmonics >> level;
    const int cap = waveLevelLength (level) / 2 - 1;
    return nominal < cap ? nominal : cap;
}

//==============================================================================
/** One mip level of one bank: `numFrames` tables laid out back to back. */
struct WavetableLevel
{
    const float* data = nullptr;    ///< numFrames * length samples
    int      length  = 0;
    uint32_t mask    = 0;
    float    lengthF = 0.0f;
    int      maxHarmonics = 0;

    inline const float* frame (int f) const noexcept { return data + (size_t) f * (size_t) length; }
};

/** One bank: a mip pyramid plus its frame count. */
struct WavetableBankData
{
    std::array<WavetableLevel, kWaveNumLevels> levels {};
    int numFrames = kWaveFramesPerBank;
    const char* name = "";
};

//==============================================================================
// Interpolation and morph helpers shared by the audio path, the display helper
// and the tests, so that all three agree by construction.

/** Wraps a phase into [0, 1) (works for negative phases from through-zero FM). */
inline float wavePhaseWrap (float p) noexcept
{
    p -= std::floor (p);
    return p >= 1.0f ? 0.0f : (p < 0.0f ? 0.0f : p);
}

/**
    MORPH — asymmetric phase warp (Casio-style phase distortion).

    morph 0 leaves the frame untouched; morph 1 compresses the first half of
    the cycle into the first 1/8 (slope 4), which pushes the spectrum up and
    adds a formant/sync-like edge. `waveMorphSlope` reports the resulting
    maximum time compression so the mip selection can stay band limited.
*/
inline float waveMorphPivot (float morph) noexcept
{
    return 0.5f - 0.375f * clamp01 (morph);
}

inline float waveMorphSlope (float morph) noexcept
{
    return 0.5f / waveMorphPivot (morph);
}

inline float waveMorphWarp (float phase, float pivot) noexcept
{
    return phase < pivot ? 0.5f * phase / pivot
                         : 0.5f + 0.5f * (phase - pivot) / (1.0f - pivot);
}

/**
    Reads two frames of the same mip level with one 4-point Catmull-Rom
    interpolation: the taps are blended first (interpolation is linear, so the
    result is identical to interpolating both frames separately) which halves
    the work of position morphing.
*/
inline float waveReadFrames (const float* a, const float* b, float blend,
                             uint32_t mask, float lengthF, float phase) noexcept
{
    const float fi = phase * lengthF;
    const int   i0 = (int) fi;
    const float f  = fi - (float) i0;

    const uint32_t im = ((uint32_t) i0 - 1u) & mask;
    const uint32_t i1 = ((uint32_t) i0) & mask;
    const uint32_t i2 = ((uint32_t) i0 + 1u) & mask;
    const uint32_t i3 = ((uint32_t) i0 + 2u) & mask;

    const float ym = a[im] + blend * (b[im] - a[im]);
    const float y0 = a[i1] + blend * (b[i1] - a[i1]);
    const float y1 = a[i2] + blend * (b[i2] - a[i2]);
    const float y2 = a[i3] + blend * (b[i3] - a[i3]);

    const float c1 = 0.5f * (y1 - ym);
    const float c2 = ym - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    const float c3 = 0.5f * (y2 - ym) + 1.5f * (y0 - y1);
    return ((c3 * f + c2) * f + c1) * f + y0;
}

/** Mip level pair + crossfade weight for a normalised increment. */
struct WaveMipSelection
{
    int   levelA = 0;
    int   levelB = 0;
    float blend  = 0.0f;   ///< 0 = levelA only, 1 = levelB only
};

/**
    Chooses the mip level for an increment (cycles per sample).

    `levelA` is always the brightest level that is guaranteed alias free; the
    top quarter of every level's range crossfades into the next (duller, also
    alias-free) level so sweeps never step.
*/
inline WaveMipSelection waveSelectMip (float inc) noexcept
{
    const float x = std::log2 (std::max (std::abs (inc), 1.0e-9f) / kWaveIncRef);
    const float lf = std::floor (x);
    const float frac = x - lf;

    WaveMipSelection s;
    const int base = (int) lf + 1;
    s.levelA = juce::jlimit (0, kWaveNumLevels - 1, base);
    s.levelB = juce::jlimit (0, kWaveNumLevels - 1, base + 1);
    s.blend  = frac > 0.75f ? (frac - 0.75f) * 4.0f : 0.0f;
    return s;
}

//==============================================================================
/**
    Access to the shared table cache.

    The cache is built on first use (message thread, from WaveSource::prepare)
    and is immutable afterwards, so the audio thread only ever reads it.
*/
class Wavetables
{
public:
    /** Builds the cache if it does not exist yet. Never call from the audio thread. */
    static void prewarm();

    static const WavetableBankData& bank (int index) noexcept;
    static const char* bankName (int index) noexcept;

    /** Total bytes held by the shared table cache. */
    static size_t memoryBytes() noexcept;

    /** 4096-point sine used by the FM / PM / AM / ring modulator. */
    static const float* sineTable() noexcept;

    /** BLEP residual table used to band-limit hard-sync resets. */
    static const float* blepTable() noexcept;

    //==========================================================================
    /**
        MESSAGE-THREAD DISPLAY HELPER (for the UI).

        Reconstructs one cycle of the frame the audio engine is currently
        playing — the same frame interpolation and the same MORPH warp — into
        `out`, `numSamples` points covering exactly one period.

        @param bankIndex   source.wave.table choice (0..7, clamped)
        @param position    source.wave.position (0..1, clamped)
        @param morph       source.wave.morph (0..1, clamped)
        @param out         destination, numSamples floats
        @param numSamples  number of points to draw (any size)

        Safe to call from the message thread at any time; builds the cache on
        first use, never allocates afterwards, never blocks the audio thread.
    */
    static void renderDisplayFrame (int bankIndex, float position, float morph,
                                    float* out, int numSamples) noexcept;

    /** Effective frame position (0..numFrames-1) for a normalised position. */
    static float framePosition (int bankIndex, float position) noexcept;
};

//==============================================================================
// Modulator sine lookup (linear interpolation on a 4096-point table: the
// residual error is below -120 dB, far under the table noise floor).

constexpr int kWaveSineSize = 4096;
constexpr uint32_t kWaveSineMask = (uint32_t) kWaveSineSize - 1u;

inline float waveSineLookup (const float* table, float phase) noexcept
{
    const float fi = phase * (float) kWaveSineSize;
    const int   i0 = (int) fi;
    const float f  = fi - (float) i0;
    const float a = table[(uint32_t) i0 & kWaveSineMask];
    const float b = table[((uint32_t) i0 + 1u) & kWaveSineMask];
    return a + f * (b - a);
}

//==============================================================================
// Band-limited step (BLEP) geometry for hard sync.

constexpr int kWaveBlepZ    = 4;                    ///< half width in samples
constexpr int kWaveBlepLen  = 2 * kWaveBlepZ;       ///< residual taps
constexpr int kWaveBlepRes  = 64;                   ///< fractional-delay subdivisions
constexpr uint32_t kWaveBlepRingMask = (uint32_t) kWaveBlepLen - 1u;

} // namespace am
