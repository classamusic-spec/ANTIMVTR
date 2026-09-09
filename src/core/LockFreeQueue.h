#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace am
{

/**
    Bounded single-producer / single-consumer queue of trivially copyable items.
    push() never blocks and never allocates; it returns false when full.
*/
template <typename T, size_t Capacity>
class SpscQueue
{
    static_assert ((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert (std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    bool push (const T& item) noexcept
    {
        const size_t w = writeIndex.load (std::memory_order_relaxed);
        const size_t r = readIndex.load (std::memory_order_acquire);
        if (w - r >= Capacity)
            return false;
        buffer[w & mask] = item;
        writeIndex.store (w + 1, std::memory_order_release);
        return true;
    }

    bool pop (T& out) noexcept
    {
        const size_t r = readIndex.load (std::memory_order_relaxed);
        const size_t w = writeIndex.load (std::memory_order_acquire);
        if (r == w)
            return false;
        out = buffer[r & mask];
        readIndex.store (r + 1, std::memory_order_release);
        return true;
    }

    size_t size() const noexcept
    {
        return writeIndex.load (std::memory_order_acquire) - readIndex.load (std::memory_order_acquire);
    }

    bool empty() const noexcept { return size() == 0; }
    static constexpr size_t capacity() noexcept { return Capacity; }

private:
    static constexpr size_t mask = Capacity - 1;
    std::array<T, Capacity> buffer {};
    std::atomic<size_t> writeIndex { 0 };
    std::atomic<size_t> readIndex { 0 };
};

/**
    Wait-free triple buffer for publishing snapshots from the audio thread
    to a reader (UI / diagnostics). The writer always has a free slot; the
    reader always sees the most recently published complete snapshot.
*/
template <typename T>
class TripleBuffer
{
    static_assert (std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    TripleBuffer() = default;

    /** Writer: obtain the slot to fill. */
    T& beginWrite() noexcept { return slots[writeSlot]; }

    /** Writer: publish the slot filled since beginWrite(). */
    void endWrite() noexcept
    {
        const uint8_t previous = middle.exchange ((uint8_t) (writeSlot | dirtyBit), std::memory_order_acq_rel);
        writeSlot = previous & indexMask;
    }

    /** Reader: returns true if a new snapshot was available and `out` was updated. */
    bool read (T& out) noexcept
    {
        if ((middle.load (std::memory_order_acquire) & dirtyBit) == 0)
            return false;
        const uint8_t previous = middle.exchange (readSlot, std::memory_order_acq_rel);
        readSlot = previous & indexMask;
        out = slots[readSlot];
        cache = out;
        return true;
    }

    /** Reader: returns the most recent snapshot ever read (reading a new one if available). */
    const T& latest() noexcept
    {
        T tmp;
        read (tmp);
        return cache;
    }

private:
    static constexpr uint8_t dirtyBit  = 0x04;
    static constexpr uint8_t indexMask = 0x03;

    T slots[3] {};
    T cache {};
    uint8_t writeSlot = 0;
    uint8_t readSlot  = 2;
    std::atomic<uint8_t> middle { 1 };
};

} // namespace am
