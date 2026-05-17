#pragma once

#include <cstdint>
#include <span>

#include "IIOBackend.h"
#include "NetworkSessionTypes.h"

// WITH_Server/Session.h의 SessionId와 동일한 타입.

// Executor → 네트워크 모듈 인터페이스.
// IIOBackend를 상속하여 IO 중립 책임(DrainCompletions, DebugName)을 공유하고,
// 네트워크 고유 송수신 책임(Send, FlushSend, Disconnect, GetSessionCount)을 추가한다.
// TaskExecutor는 _ioBackends(vector<IIOBackend*>)로 이 인터페이스를 관리하며,
// SetNetworkBackend()는 RegisterIOBackend()로 위임한다.
struct INetworkBackend : IIOBackend
{
    // Reconcile Phase: 세션에 페이로드 전송.
    virtual bool Send(SessionId sessionId,
                      std::span<const uint8_t> payload) noexcept = 0;

    // Reconcile Phase 말미: 버퍼 플러시.
    // Asio : no-op (Connection 내부 gather-write 자동 처리)
    // IOCP : WSASend() 일괄 게시
    virtual void FlushSend() noexcept = 0;

    // 세션 강제 종료 (인증 실패, 킥 등). DynamicTask 핸들러에서 호출.
    virtual void Disconnect(
        SessionId sessionId,
        SessionCloseReason reason = SessionCloseReason::LocalRequested) noexcept = 0;

    virtual uint32_t GetSessionCount() const noexcept = 0;
};
