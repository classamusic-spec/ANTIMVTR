#pragma once

#include "core/LockFreeQueue.h"
#include "core/Types.h"

namespace am
{

enum class EngineEventType : uint8_t
{
    EnginePrepared = 0,
    EngineReset,
    VoiceStarted,
    VoiceEnded,
    VoiceStolen,
    PolyphonyChanged,
    QualityChanged,
    TopologyRegenerated,
    MaterialChanged,
    FreezeCaptured,
    FFTResized,
    SampleAnalysisComplete,
    SafetyEvent,
    SafetyReset,
    PresetLoaded,
    LatencyChanged,
    Custom,
    Count
};

/** A small POD event that can be pushed from the audio thread without allocation. */
struct EngineEvent
{
    EngineEventType type   = EngineEventType::Custom;
    uint8_t  subsystem     = (uint8_t) Subsystem::Unknown;
    int16_t  voice         = -1;
    uint32_t a             = 0;     ///< generic integer payload (e.g. note number, safety event id)
    float    f             = 0.0f;  ///< generic float payload
    uint64_t sampleTime    = 0;     ///< engine sample clock when the event was raised
};

/**
    Lock-free single-producer (audio thread) / single-consumer (message thread)
    event log. Events that do not fit are dropped and counted.
*/
class EngineEventQueue
{
public:
    bool push (const EngineEvent& e) noexcept
    {
        if (queue.push (e)) return true;
        dropped.fetch_add (1, std::memory_order_relaxed);
        return false;
    }

    bool push (EngineEventType type, Subsystem s, int voice, uint32_t a, float f, uint64_t sampleTime) noexcept
    {
        EngineEvent e;
        e.type = type; e.subsystem = (uint8_t) s; e.voice = (int16_t) voice; e.a = a; e.f = f; e.sampleTime = sampleTime;
        return push (e);
    }

    /** Drains all pending events on the consumer thread. */
    template <typename Fn>
    void drain (Fn&& fn)
    {
        EngineEvent e;
        while (queue.pop (e))
            fn (e);
    }

    uint32_t droppedCount() const noexcept { return dropped.load (std::memory_order_relaxed); }
    size_t   pending() const noexcept      { return queue.size(); }

    static const char* typeName (EngineEventType t) noexcept;

    /** Produces a human readable line for the developer log. Message thread only. */
    static juce::String describe (const EngineEvent& e);

private:
    SpscQueue<EngineEvent, 2048> queue;
    std::atomic<uint32_t> dropped { 0 };
};

} // namespace am
