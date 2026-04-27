#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <utility>

/*
    Bounded Chase-Lev work-stealing deque.

    Ownership model:
    - The owning worker is the only thread allowed to call TryPush/TryPop.
    - Other workers may call TrySteal concurrently.
    - Reset must only be called after all producers/consumers for the frame are idle.

    This is a fixed-capacity variant. It does not grow the backing array, so one
    slot is kept empty to distinguish full from empty in the circular buffer.
*/
template<typename T, int64_t Capacity>
class LFWSDeque {
    static constexpr int64_t kMask{ Capacity - 1 };
    static_assert(Capacity > 1, "Capacity must be greater than 1");
    static_assert((Capacity & kMask) == 0, "Capacity must be power of 2");

public:
    [[nodiscard]] bool TryPush(const T& item);
    [[nodiscard]] bool TryPush(T&& item);

    [[nodiscard]] std::optional<T> TryPop();
    [[nodiscard]] std::optional<T> TrySteal();

    void Reset() noexcept;

private:
    alignas(64) std::atomic<int64_t> _top{ 0 };
    alignas(64) std::atomic<int64_t> _bottom{ 0 };

    alignas(64) std::atomic<T> _buffer[Capacity];
};

template<typename T, int64_t Capacity>
inline bool LFWSDeque<T, Capacity>::TryPush(const T& item)
{
    const int64_t bottom = _bottom.load(std::memory_order_relaxed);
    const int64_t top = _top.load(std::memory_order_acquire);

    if (bottom - top >= Capacity - 1)
        return false;

    _buffer[bottom & kMask].store(item, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    _bottom.store(bottom + 1, std::memory_order_release);
    return true;
}

template<typename T, int64_t Capacity>
inline bool LFWSDeque<T, Capacity>::TryPush(T&& item)
{
    const int64_t bottom = _bottom.load(std::memory_order_relaxed);
    const int64_t top = _top.load(std::memory_order_acquire);

    if (bottom - top >= Capacity - 1)
        return false;

    _buffer[bottom & kMask].store(std::move(item), std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    _bottom.store(bottom + 1, std::memory_order_release);
    return true;
}

template<typename T, int64_t Capacity>
inline std::optional<T> LFWSDeque<T, Capacity>::TryPop()
{
    const int64_t bottom = _bottom.load(std::memory_order_relaxed) - 1;
    _bottom.store(bottom, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    int64_t top = _top.load(std::memory_order_acquire);

    if (bottom < top)
    {
        _bottom.store(bottom + 1, std::memory_order_relaxed);
        return std::nullopt;
    }

    T item = _buffer[bottom & kMask].load(std::memory_order_relaxed);

    if (bottom > top)
        return item;

    const int64_t nextTop = top + 1;
    if (_top.compare_exchange_strong(
        top, nextTop,
        std::memory_order_seq_cst,
        std::memory_order_relaxed))
    {
        _bottom.store(nextTop, std::memory_order_relaxed);
        return item;
    }

    // CAS failure means a thief took the last item. compare_exchange updates
    // 'top' on failure, so restore from the saved value instead of top + 1.
    _bottom.store(nextTop, std::memory_order_relaxed);
    return std::nullopt;
}

template<typename T, int64_t Capacity>
inline std::optional<T> LFWSDeque<T, Capacity>::TrySteal()
{
    int64_t top = _top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    const int64_t bottom = _bottom.load(std::memory_order_acquire);

    if (top >= bottom)
        return std::nullopt;

    T item = _buffer[top & kMask].load(std::memory_order_relaxed);

    if (!_top.compare_exchange_strong(
        top, top + 1,
        std::memory_order_seq_cst,
        std::memory_order_relaxed))
    {
        return std::nullopt;
    }

    return item;
}

template<typename T, int64_t Capacity>
inline void LFWSDeque<T, Capacity>::Reset() noexcept
{
    _top.store(0, std::memory_order_relaxed);
    _bottom.store(0, std::memory_order_relaxed);
}
