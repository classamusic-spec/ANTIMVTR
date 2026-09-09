#pragma once

#include "LockFreeQueue.h"
#include <memory>

namespace am
{

/**
    Hands immutable data objects from the message thread to the audio thread
    without locks or audio-thread allocation.

    Message thread:  handoff.publish (std::make_unique<T> (...));  // any time
                     handoff.collectGarbage();                     // periodically (timer)
    Audio thread:    const T* data = handoff.acquire();            // once per block

    The audio thread swaps in the newest pending object and parks the previous
    one for the message thread to delete. Objects are never freed on the audio
    thread. If the message thread publishes faster than the audio thread
    consumes, intermediate objects are dropped (deleted on the message thread).
*/
template <typename T>
class RealtimeHandoff
{
public:
    RealtimeHandoff() = default;
    ~RealtimeHandoff()
    {
        delete pending.exchange (nullptr);
        delete current;
        collectGarbage();
    }

    /** Message thread. Publishes a new object; the audio thread picks it up on its next acquire(). */
    void publish (std::unique_ptr<T> object)
    {
        T* previous = pending.exchange (object.release(), std::memory_order_acq_rel);
        delete previous;   // never seen by the audio thread — safe to free here
    }

    /** Audio thread. Returns the current object (may be null before the first publish). */
    const T* acquire() noexcept
    {
        T* fresh = pending.exchange (nullptr, std::memory_order_acq_rel);
        if (fresh != nullptr)
        {
            if (current != nullptr && ! garbage.push (current))
            {
                // Garbage queue full: keep the old object alive in an overflow slot
                // rather than leaking or freeing on the audio thread.
                T* spilled = overflow.exchange (current, std::memory_order_acq_rel);
                if (spilled != nullptr) leaked.fetch_add (1, std::memory_order_relaxed), (void) spilled;
            }
            current = fresh;
        }
        return current;
    }

    /** Message thread. Frees objects the audio thread has finished with. */
    void collectGarbage()
    {
        T* dead = nullptr;
        while (garbage.pop (dead)) delete dead;
        delete overflow.exchange (nullptr, std::memory_order_acq_rel);
    }

    /** Message-thread convenience: the last object published (may already be superseded). */
    bool hasPending() const noexcept { return pending.load (std::memory_order_acquire) != nullptr; }

private:
    std::atomic<T*> pending { nullptr };
    std::atomic<T*> overflow { nullptr };
    std::atomic<uint32_t> leaked { 0 };
    T* current = nullptr;                 // audio thread only
    SpscQueue<T*, 64> garbage;            // audio → message
};

} // namespace am
