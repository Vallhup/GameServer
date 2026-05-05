#pragma once

#include <chrono>
#include <cstdint>
#include <span>

// WITH_Server/Session.h의 SessionId와 동일한 타입.
using SessionId = uint32_t;

// Executor → 네트워크 모듈 인터페이스.
// TaskExecutor는 이 인터페이스만 알고 구체 구현(Asio/IOCP)을 모른다.
struct INetworkBackend
{
    // WorkerPump에서 작업이 없을 때 호출.
    // Asio : cv.wait_for(timeout)  — IO 처리는 별도 IO 스레드에서 수행.
    // IOCP : GetQueuedCompletionStatusEx() + 완료 처리 후 반환.
    virtual void WaitForWork(uint32_t workerIdx,
                              std::chrono::microseconds timeout) noexcept = 0;

    // TaskExecutor::PushCompletion()이 내부적으로 호출. 대기 워커 1개 깨우기.
    // Asio : cv.notify_one()
    // IOCP : PostQueuedCompletionStatus()
    virtual void WakeWorker() noexcept = 0;

    // Reconcile Phase: 세션에 페이로드 전송.
    virtual bool Send(SessionId sessionId,
                      std::span<const uint8_t> payload) noexcept = 0;

    // Reconcile Phase 말미: 버퍼 플러시.
    // Asio : no-op (Connection 내부 gather-write 자동 처리)
    // IOCP : WSASend() 일괄 게시
    virtual void FlushSend() noexcept = 0;

    // 세션 강제 종료 (인증 실패, 킥 등). DynamicTask 핸들러에서 호출.
    virtual void Disconnect(SessionId sessionId) noexcept = 0;

    virtual uint32_t GetSessionCount() const noexcept = 0;

    virtual ~INetworkBackend() = default;
};
