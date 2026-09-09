#pragma once

#include "EngineEventQueue.h"

namespace am
{

enum class SafetyEvent : uint8_t
{
    NaN = 0,
    Infinity,
    Denormal,
    HardClip,
    LimiterEngaged,
    DCOffset,
    FeedbackClamp,
    InvalidFrequency,
    InvalidCoefficient,
    ResonatorReset,
    Count
};

/**
    Real-time safe counters for numerical / level safety events.

    Any subsystem may call note() from the audio thread. Counters are atomics;
    the first few occurrences per category also produce EngineEvents so the
    developer log shows where instability began (later ones are only counted
    to avoid flooding the queue).
*/
class SafetyMonitor
{
public:
    static constexpr int kNumEvents = (int) SafetyEvent::Count;

    struct Counters
    {
        uint32_t counts[kNumEvents] {};
        int16_t  lastVoice[kNumEvents] {};
        uint8_t  lastSubsystem[kNumEvents] {};
        uint32_t total = 0;
    };

    void attachEventQueue (EngineEventQueue* q) noexcept { events = q; }
    void setSampleClock (const std::atomic<uint64_t>* clock) noexcept { sampleClock = clock; }

    void note (SafetyEvent e, Subsystem s, int voice = -1, int count = 1) noexcept
    {
        const auto i = (size_t) e;
        const uint32_t previous = counters[i].fetch_add ((uint32_t) count, std::memory_order_relaxed);
        lastVoice[i].store ((int16_t) voice, std::memory_order_relaxed);
        lastSubsystem[i].store ((uint8_t) s, std::memory_order_relaxed);

        if (events != nullptr && previous < kEventsLoggedPerCategory)
            events->push (EngineEventType::SafetyEvent, s, voice, (uint32_t) e, (float) count,
                          sampleClock != nullptr ? sampleClock->load (std::memory_order_relaxed) : 0);
    }

    uint32_t count (SafetyEvent e) const noexcept { return counters[(size_t) e].load (std::memory_order_relaxed); }

    uint32_t total() const noexcept
    {
        uint32_t t = 0;
        for (const auto& c : counters) t += c.load (std::memory_order_relaxed);
        return t;
    }

    Counters snapshot() const noexcept
    {
        Counters c;
        for (int i = 0; i < kNumEvents; ++i)
        {
            c.counts[i]        = counters[(size_t) i].load (std::memory_order_relaxed);
            c.lastVoice[i]     = lastVoice[(size_t) i].load (std::memory_order_relaxed);
            c.lastSubsystem[i] = lastSubsystem[(size_t) i].load (std::memory_order_relaxed);
            c.total += c.counts[i];
        }
        return c;
    }

    void reset() noexcept
    {
        for (auto& c : counters) c.store (0, std::memory_order_relaxed);
        if (events != nullptr)
            events->push (EngineEventType::SafetyReset, Subsystem::Unknown, -1, 0, 0.0f,
                          sampleClock != nullptr ? sampleClock->load (std::memory_order_relaxed) : 0);
    }

    static const char* eventName (SafetyEvent e) noexcept
    {
        switch (e)
        {
            case SafetyEvent::NaN:                return "NaN";
            case SafetyEvent::Infinity:           return "Infinity";
            case SafetyEvent::Denormal:           return "Denormal";
            case SafetyEvent::HardClip:           return "Hard clip";
            case SafetyEvent::LimiterEngaged:     return "Limiter engaged";
            case SafetyEvent::DCOffset:           return "DC offset";
            case SafetyEvent::FeedbackClamp:      return "Feedback clamp";
            case SafetyEvent::InvalidFrequency:   return "Invalid frequency";
            case SafetyEvent::InvalidCoefficient: return "Invalid coefficient";
            case SafetyEvent::ResonatorReset:     return "Resonator reset";
            default:                              return "Unknown";
        }
    }

private:
    static constexpr uint32_t kEventsLoggedPerCategory = 8;

    std::array<std::atomic<uint32_t>, kNumEvents> counters {};
    std::array<std::atomic<int16_t>,  kNumEvents> lastVoice {};
    std::array<std::atomic<uint8_t>,  kNumEvents> lastSubsystem {};
    EngineEventQueue* events = nullptr;
    const std::atomic<uint64_t>* sampleClock = nullptr;
};

} // namespace am
