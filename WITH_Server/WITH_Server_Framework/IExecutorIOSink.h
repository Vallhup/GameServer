#pragma once

#include "AsyncIOTypes.h"      // CompletionEntry
#include "DynamicTaskTypes.h"  // DynamicTaskRequest

// 네트워크 모듈 → Executor 인터페이스.
// 네트워크 모듈은 이 인터페이스만 알고 TaskGraph 내부를 모른다.
struct IExecutorIOSink
{
    // 인바운드 패킷 → 다음 프레임 DynamicTask 주입 (thread-safe).
    // 네트워크 모듈(IO 스레드/워커)에서 호출.
    // 주입된 요청은 다음 프레임 빌드 시 처리되므로 WakeWorker 불필요.
    virtual void SubmitDynamicTask(DynamicTaskRequest request) noexcept = 0;

    // 아웃바운드 IO 완료 → Suspended 노드 재개 (thread-safe).
    // 내부에서 INetworkBackend::WakeWorker()를 자동 호출하여 대기 워커를 깨운다.
    virtual void PushCompletion(CompletionEntry entry) noexcept = 0;

    virtual ~IExecutorIOSink() = default;
};
