#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

// Worker가 idle일 때 기다리는 단일 sleep 지점.
//
// 설계 원칙 (Spec §3.5 / §3.7):
//   - OS 스케줄러 합의 "wait는 한 곳, wake는 어디서든"을 따른다.
//     IOCP / Asio / DB 등 모든 IO backend는 자기 작업 도착 시 Wake() 호출만 한다.
//   - Searching worker 최적화 (Tokio): 이미 task를 탐색 중인 worker가 있으면
//     추가 unpark을 생략한다. EnterSearching / ExitSearching으로 카운터를 관리한다.
//   - _wakeTokens: spurious wakeup 및 lost wake 방지용 토큰.
//     WaitForWakeup이 토큰을 소비하고, Wake/WakeN이 토큰을 공급한다.
//
// 1차 구현은 std::condition_variable. 향후 IOCP 환경에서
// coordinator의 wake 메커니즘을 completion port 자체로 단일화하는 옵션이 있다.
class ExecutorIdleCoordinator final
{
public:
    ExecutorIdleCoordinator() = default;
    ~ExecutorIdleCoordinator() = default;

    ExecutorIdleCoordinator(const ExecutorIdleCoordinator&) = delete;
    ExecutorIdleCoordinator& operator=(const ExecutorIdleCoordinator&) = delete;

    // worker가 작업이 없을 때 호출한다.
    // - wake 신호(토큰)를 받으면 즉시 true 반환.
    // - timeout 경과까지 신호가 없으면 false 반환.
    // ExitSearching은 이 함수 진입 전에 호출자가 직접 호출해야 한다.
    [[nodiscard]]
    bool WaitForWakeup(uint32_t workerIdx,
                       std::chrono::microseconds timeout) noexcept;

    // 어느 backend · 어느 thread에서든 호출 가능 (thread-safe).
    // 이미 searching 상태인 worker가 있으면 wake를 생략한다 (Tokio 최적화).
    void Wake() noexcept;

    // 여러 completion이 한 번에 도착했을 때 사용 (FUTEX_CMP_REQUEUE 의미).
    // count만큼 worker를 깨운다.
    void WakeN(uint32_t count) noexcept;

    // WorkerPump가 task lookup phase(Phase 1·2·3 순회)에 진입할 때 호출.
    // searching worker가 있으면 Wake()가 추가 unpark을 생략한다.
    void EnterSearching(uint32_t workerIdx) noexcept;

    // task를 찾았거나 WaitForWakeup으로 진입하기 직전에 호출.
    void ExitSearching(uint32_t workerIdx) noexcept;

    // Shutdown 시 대기 중인 모든 worker를 깨운다.
    void WakeAllForShutdown() noexcept;

private:
    std::mutex               _mutex;
    std::condition_variable  _cv;
    std::atomic<uint32_t>    _searchingWorkers{ 0 };
    std::atomic<uint32_t>    _wakeTokens{ 0 };
};
