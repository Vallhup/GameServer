#include "pch.h"
#include "ExecutorIdleCoordinator.h"

bool ExecutorIdleCoordinator::WaitForWakeup(
    uint32_t /*workerIdx*/,
    std::chrono::microseconds timeout) noexcept
{
    // 토큰이 이미 적립돼 있으면 sleep 없이 즉시 소비하고 반환한다.
    // memory_order_acquire: 토큰을 게시한 쪽의 store가 이 load보다 먼저 보인다.
    uint32_t tokens = _wakeTokens.load(std::memory_order_acquire);
    if (tokens > 0)
    {
        if (_wakeTokens.compare_exchange_strong(
                tokens, tokens - 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed))
        {
            return true;
        }
    }

    std::unique_lock lock{ _mutex };

    // lock 획득 후 재확인 — lock과 atomic 사이의 lost-wake 방지.
    tokens = _wakeTokens.load(std::memory_order_acquire);
    if (tokens > 0)
    {
        _wakeTokens.fetch_sub(1, std::memory_order_acq_rel);
        return true;
    }

    const bool woken = _cv.wait_for(
        lock,
        timeout,
        [this]()
        {
            return _wakeTokens.load(std::memory_order_relaxed) > 0;
        });

    if (woken)
        _wakeTokens.fetch_sub(1, std::memory_order_acq_rel);

    return woken;
}

void ExecutorIdleCoordinator::Wake() noexcept
{
    // Tokio 최적화: 이미 task lookup phase에 있는 worker가 존재하면
    // 추가 unpark 비용을 치르지 않는다. 그 worker가 곧 처리할 것이다.
    if (_searchingWorkers.load(std::memory_order_relaxed) > 0)
        return;

    _wakeTokens.fetch_add(1, std::memory_order_release);
    _cv.notify_one();
}

void ExecutorIdleCoordinator::WakeN(uint32_t count) noexcept
{
    if (count == 0)
        return;

    _wakeTokens.fetch_add(count, std::memory_order_release);

    // count가 1이면 notify_one, 그 이상이면 notify_all.
    // (FUTEX_CMP_REQUEUE 의미 — 필요한 수만큼만 깨운다)
    if (count == 1)
        _cv.notify_one();
    else
        _cv.notify_all();
}

void ExecutorIdleCoordinator::EnterSearching(uint32_t /*workerIdx*/) noexcept
{
    _searchingWorkers.fetch_add(1, std::memory_order_relaxed);
}

void ExecutorIdleCoordinator::ExitSearching(uint32_t /*workerIdx*/) noexcept
{
    _searchingWorkers.fetch_sub(1, std::memory_order_relaxed);
}

void ExecutorIdleCoordinator::WakeAllForShutdown() noexcept
{
    // shutdown 경로: 토큰 없이 notify_all만으로 모든 대기 worker를 깨운다.
    // WaitForWakeup의 predicate가 false여도 _cv.wait_for가 반환하도록
    // 충분히 큰 토큰을 적립한다.
    _wakeTokens.fetch_add(0xFFFF, std::memory_order_release);
    _cv.notify_all();
}
