#pragma once

#include "core/Types.h"
#include <chrono>

#ifndef ANTIMATR_PROFILING
 #define ANTIMATR_PROFILING ANTIMATR_DEV
#endif

namespace am
{

/**
    Per-subsystem CPU profiler for the audio callback.

    Usage on the audio thread:
        profiler.beginBlock (numSamples);
        { PerformanceProfiler::Scoped t (profiler, Subsystem::Matter); ... }
        profiler.endBlock();

    Timing uses std::chrono::steady_clock (tens of nanoseconds per call) and is
    compiled out entirely when ANTIMATR_PROFILING is 0. Statistics are published
    through atomics so DSP LAB can read them from the message thread.
*/
class PerformanceProfiler
{
public:
    static constexpr int kNumSubsystems = (int) Subsystem::Count;

    struct Stats
    {
        float avgPercent[kNumSubsystems] {};     ///< long-run average of block budget used
        float peakPercent[kNumSubsystems] {};    ///< worst block seen since reset
        float movingPercent[kNumSubsystems] {};  ///< exponential moving average (fast)
        float totalAvgPercent    = 0.0f;
        float totalPeakPercent   = 0.0f;
        float totalMovingPercent = 0.0f;
        double lastBlockMicros   = 0.0;
        double budgetMicros      = 0.0;
        double sampleRate        = 0.0;
        int    blockSize         = 0;
        int    activeVoices      = 0;
        uint32_t blocksMeasured  = 0;
        uint32_t overruns        = 0;            ///< blocks that exceeded 100% of budget
    };

    void prepare (double sampleRate, int blockSize) noexcept
    {
        sr = sampleRate;
        reset();
        stats.sampleRate.store (sampleRate, std::memory_order_relaxed);
        stats.blockSize.store (blockSize, std::memory_order_relaxed);
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumSubsystems; ++i)
        {
            accum[i] = 0;
            stats.avg[i].store (0.0f, std::memory_order_relaxed);
            stats.peak[i].store (0.0f, std::memory_order_relaxed);
            stats.moving[i].store (0.0f, std::memory_order_relaxed);
        }
        stats.totalAvg.store (0.0f); stats.totalPeak.store (0.0f); stats.totalMoving.store (0.0f);
        stats.blocks.store (0); stats.overruns.store (0);
        blocksSeen = 0;
    }

    void setEnabled (bool e) noexcept { enabled.store (e, std::memory_order_relaxed); }
    bool isEnabled() const noexcept   { return enabled.load (std::memory_order_relaxed); }

    void setActiveVoices (int v) noexcept { stats.voices.store (v, std::memory_order_relaxed); }

    void beginBlock (int numSamples) noexcept
    {
       #if ANTIMATR_PROFILING
        if (! isEnabled()) return;
        blockSamples = numSamples;
        for (auto& a : accum) a = 0;
        blockStart = now();
       #else
        juce::ignoreUnused (numSamples);
       #endif
    }

    void add (Subsystem s, int64_t nanoseconds) noexcept
    {
       #if ANTIMATR_PROFILING
        accum[(size_t) s] += nanoseconds;
       #else
        juce::ignoreUnused (s, nanoseconds);
       #endif
    }

    void endBlock() noexcept
    {
       #if ANTIMATR_PROFILING
        if (! isEnabled() || sr <= 0.0 || blockSamples <= 0) return;

        const int64_t totalNs = now() - blockStart;
        const double budgetNs = (double) blockSamples / sr * 1.0e9;
        const double toPercent = 100.0 / budgetNs;

        ++blocksSeen;
        const float alphaAvg = 1.0f / (float) std::min<uint64_t> (blocksSeen, 4096);
        constexpr float alphaMoving = 0.08f;

        float subsystemSum = 0.0f;
        for (int i = 0; i < kNumSubsystems; ++i)
        {
            const float pct = (float) ((double) accum[i] * toPercent);
            subsystemSum += pct;
            stats.avg[i].store (stats.avg[i].load (std::memory_order_relaxed) + alphaAvg * (pct - stats.avg[i].load (std::memory_order_relaxed)), std::memory_order_relaxed);
            stats.moving[i].store (stats.moving[i].load (std::memory_order_relaxed) + alphaMoving * (pct - stats.moving[i].load (std::memory_order_relaxed)), std::memory_order_relaxed);
            if (pct > stats.peak[i].load (std::memory_order_relaxed)) stats.peak[i].store (pct, std::memory_order_relaxed);
        }
        juce::ignoreUnused (subsystemSum);

        const float totalPct = (float) ((double) totalNs * toPercent);
        stats.totalAvg.store (stats.totalAvg.load (std::memory_order_relaxed) + alphaAvg * (totalPct - stats.totalAvg.load (std::memory_order_relaxed)), std::memory_order_relaxed);
        stats.totalMoving.store (stats.totalMoving.load (std::memory_order_relaxed) + alphaMoving * (totalPct - stats.totalMoving.load (std::memory_order_relaxed)), std::memory_order_relaxed);
        if (totalPct > stats.totalPeak.load (std::memory_order_relaxed)) stats.totalPeak.store (totalPct, std::memory_order_relaxed);
        if (totalPct > 100.0f) stats.overruns.fetch_add (1, std::memory_order_relaxed);

        stats.lastBlockMicros.store ((double) totalNs * 1.0e-3, std::memory_order_relaxed);
        stats.budgetMicros.store (budgetNs * 1.0e-3, std::memory_order_relaxed);
        stats.blocks.store ((uint32_t) blocksSeen, std::memory_order_relaxed);
       #endif
    }

    Stats snapshot() const noexcept
    {
        Stats s;
        for (int i = 0; i < kNumSubsystems; ++i)
        {
            s.avgPercent[i]    = stats.avg[i].load (std::memory_order_relaxed);
            s.peakPercent[i]   = stats.peak[i].load (std::memory_order_relaxed);
            s.movingPercent[i] = stats.moving[i].load (std::memory_order_relaxed);
        }
        s.totalAvgPercent    = stats.totalAvg.load (std::memory_order_relaxed);
        s.totalPeakPercent   = stats.totalPeak.load (std::memory_order_relaxed);
        s.totalMovingPercent = stats.totalMoving.load (std::memory_order_relaxed);
        s.lastBlockMicros    = stats.lastBlockMicros.load (std::memory_order_relaxed);
        s.budgetMicros       = stats.budgetMicros.load (std::memory_order_relaxed);
        s.sampleRate         = stats.sampleRate.load (std::memory_order_relaxed);
        s.blockSize          = stats.blockSize.load (std::memory_order_relaxed);
        s.activeVoices       = stats.voices.load (std::memory_order_relaxed);
        s.blocksMeasured     = stats.blocks.load (std::memory_order_relaxed);
        s.overruns           = stats.overruns.load (std::memory_order_relaxed);
        return s;
    }

    /** RAII timer adding the elapsed time of a scope to a subsystem. */
    struct Scoped
    {
        Scoped (PerformanceProfiler& p, Subsystem s) noexcept : profiler (p), subsystem (s)
        {
           #if ANTIMATR_PROFILING
            start = now();
           #endif
        }
        ~Scoped() noexcept
        {
           #if ANTIMATR_PROFILING
            profiler.add (subsystem, now() - start);
           #endif
        }
        PerformanceProfiler& profiler;
        Subsystem subsystem;
        int64_t start = 0;
    };

    static inline int64_t now() noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::steady_clock::now().time_since_epoch()).count();
    }

private:
    struct AtomicStats
    {
        std::array<std::atomic<float>, kNumSubsystems> avg {}, peak {}, moving {};
        std::atomic<float> totalAvg { 0.0f }, totalPeak { 0.0f }, totalMoving { 0.0f };
        std::atomic<double> lastBlockMicros { 0.0 }, budgetMicros { 0.0 }, sampleRate { 0.0 };
        std::atomic<int> blockSize { 0 }, voices { 0 };
        std::atomic<uint32_t> blocks { 0 }, overruns { 0 };
    } stats;

    std::array<int64_t, kNumSubsystems> accum {};
    std::atomic<bool> enabled { true };
    double  sr = 0.0;
    int     blockSamples = 0;
    int64_t blockStart = 0;
    uint64_t blocksSeen = 0;
};

} // namespace am
