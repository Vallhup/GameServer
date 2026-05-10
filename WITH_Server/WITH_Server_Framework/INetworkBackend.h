#pragma once

#include <chrono>
#include <cstdint>
#include <span>

#include "IIOBackend.h"

// WITH_Server/Session.h의 SessionId와 동일한 타입.
using SessionId = uint32_t;

// Executor → 네트워크 모듈 인터페이스.
// IIOBackend를 상속하여 IO 중립 책임(DrainCompletions, DebugName)을 공유하고,
// 네트워크 고유 송수신 책임(Send, FlushSend, Disconnect, GetSessionCount)을 추가한다.
//
// 호환성:
//   - 기존 WaitForWork / WakeWorker는 Step 4(ExecutorIdleCoordinator 도입) 이전까지
//     호환을 위해 유지한다. Step 4 완료 후 제거 예정.
//   - TaskExecutor는 현재 _networkBackend(INetworkBackend*) 슬롯으로 이 인터페이스를
//     보유하며, Step 4에서 _ioBackends(vector<IIOBackend*>)로 전환된다.
struct INetworkBackend : IIOBackend
{
    // [Step 4 이전 호환용] WorkerPump에서 작업이 없을 때 호출.
    // Asio : cv.wait_for(timeout)  — IO 처리는 별도 IO 스레드에서 수행.
    // IOCP : GetQueuedCompletionStatusEx() + 완료 처리 후 반환.
    // Step 4 완료 후 DrainCompletions + ExecutorIdleCoordinator로 대체 예정.
    [[nodiscard]]
    virtual bool WaitForWork(uint32_t workerIdx,
                              std::chrono::microseconds timeout) noexcept = 0;

    // [Step 4 이전 호환용] TaskExecutor::PushCompletion()이 내부적으로 호출. 대기 워커 1개 깨우기.
    // Step 4 완료 후 ExecutorIdleCoordinator::Wake()로 대체 예정.
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
};
