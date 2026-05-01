#pragma once

#include <atomic>
#include <array>
#include <cassert>
#include <immintrin.h>

// ---------------------------------------------------------------------------
// Overflow Policies
// ---------------------------------------------------------------------------
template<typename T>
struct OverflowPolicy_Nullptr 
{
    template<typename... Args>
    static T* OnOverflow(Args&&...) noexcept { return nullptr; }
    static void OnExternalRelease(T*) noexcept { assert(false); }
};

template<typename T>
struct OverflowPolicy_Heap
{
    template<typename... Args>
    static T* OnOverflow(Args&&... args) { return new T(std::forward<Args>(args)...); }
    static void OnExternalRelease(T* obj) noexcept { delete obj; }
};

// ---------------------------------------------------------------------------
// PoolStats
// ---------------------------------------------------------------------------
struct PoolStats 
{
    std::atomic<uint32_t> inUse{ 0 };
    std::atomic<uint32_t> overflowCount{ 0 };
    std::atomic<uint32_t> highWaterMark{ 0 };
};

// ---------------------------------------------------------------------------
// ObjectPool
// ---------------------------------------------------------------------------
template<typename T, size_t Capacity, typename OverflowPolicy = OverflowPolicy_Nullptr<T >>
class ObjectPool {
    static constexpr uint32_t kInvalidIndex = static_cast<uint32_t>(Capacity);

    // 비사용 슬롯: nextFreeIdx 활성
    // 사용 중 슬롯: data 활성
    struct ObjectSlot 
    {
        union 
        {
            alignas(T) std::byte data[sizeof(T)];
            uint32_t             nextFreeIdx;
        };
    };

    // [32bit version | 32bit index] — ABA 방지
    union TaggedIdx 
    {
        struct { uint32_t idx; uint32_t ver; };
        uint64_t raw;

        TaggedIdx() : raw(0) {}
        TaggedIdx(uint32_t i, uint32_t v) : idx(i), ver(v) {}
    };

    // CAS 경합 완화를 위한 BackOff
    struct BackOff 
    {
        int limit;
        int maxDelay;

        BackOff() : limit(1), maxDelay(16) {}
        void Spin() 
        {
            for (int i = 0, n = limit; i < n; ++i)
                _mm_pause();

            if (limit < maxDelay) limit *= 2;
        }
    };

public:
    ObjectPool();
    ~ObjectPool() = default;

    template<typename... Args>
    [[nodiscard]] T* Acquire(Args&&... args);
    void Release(T* obj) noexcept;

    const PoolStats& Stats() const noexcept { return _stats; }
    uint32_t         Capacity() const noexcept { return static_cast<uint32_t>(Capacity); }

private:
    bool IsFromPool(T* obj) const noexcept 
    {
        auto* p = reinterpret_cast<ObjectSlot*>(obj);
        return p >= &_slots[0] && p < &_slots[Capacity];
    }

private:
    std::array<ObjectSlot, Capacity> _slots;
    std::atomic<uint64_t>            _freeHead;
    PoolStats                        _stats;
};

template<typename T, size_t Capacity, typename OverflowPolicy>
inline ObjectPool<T, Capacity, OverflowPolicy>::ObjectPool()
{
    for (uint32_t i = 0; i < Capacity; ++i)
        _slots[i].nextFreeIdx = i + 1;

    TaggedIdx head(0, 0);
    _freeHead.store(head.raw, std::memory_order_relaxed);
}

template<typename T, size_t Capacity, typename OverflowPolicy>
template<typename ...Args>
inline T* ObjectPool<T, Capacity, OverflowPolicy>::Acquire(Args && ...args)
{
    BackOff backoff;
    TaggedIdx cur, next;
    cur.raw = _freeHead.load(std::memory_order_acquire);

    while (true)
    {
        if (cur.idx >= kInvalidIndex)
        {
            ++_stats.overflowCount;
            return OverflowPolicy::OnOverflow(std::forward<Args>(args)...);
        }

        next.idx = _slots[cur.idx].nextFreeIdx;
        next.ver = cur.ver + 1;

        if (_freeHead.compare_exchange_strong(
            cur.raw, next.raw,
            std::memory_order_release,
            std::memory_order_acquire))
        {
            break;
        }

        backoff.Spin();
    }

    T* obj = new (&_slots[cur.idx].data) T(std::forward<Args>(args)...);

    uint32_t inUse = ++_stats.inUse;
    uint32_t hwm = _stats.highWaterMark.load(std::memory_order_relaxed);
    while (inUse > hwm &&
        !_stats.highWaterMark.compare_exchange_strong(hwm, inUse, std::memory_order_relaxed));

    return obj;
}

template<typename T, size_t Capacity, typename OverflowPolicy>
inline void ObjectPool<T, Capacity, OverflowPolicy>::Release(T* obj) noexcept
{
    if (obj == nullptr) return;

    if (!IsFromPool(obj))
    {
        OverflowPolicy::OnExternalRelease(obj);
        return;
    }

    obj->~T();

    // 인덱스 계산: ObjectSlot 단위 포인터 산술
    uint32_t idx = static_cast<uint32_t>(
        reinterpret_cast<ObjectSlot*>(obj) - &_slots[0]);

    BackOff backoff;
    TaggedIdx cur, next;
    cur.raw = _freeHead.load(std::memory_order_acquire);

    while (true)
    {
        _slots[idx].nextFreeIdx = cur.idx;
        next.idx = idx;
        next.ver = cur.ver + 1;

        if (_freeHead.compare_exchange_strong(
            cur.raw, next.raw,
            std::memory_order_release,
            std::memory_order_acquire))
        {
            break;
        }

        backoff.Spin();
    }

    --_stats.inUse;
}