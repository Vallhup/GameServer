#pragma once

#include "AsyncIOTypes.h"      // CompletionEntry
#include "DynamicTaskTypes.h"  // DynamicTaskRequest

// IO backend → Executor 인터페이스.
// IO backend(네트워크, DB, File 등)는 이 인터페이스만 알고 TaskGraph 내부를 모른다.
struct IExecutorIOSink
{
    // 인바운드 외부 이벤트(패킷, DB 응답 등) → 다음 프레임 DynamicTask 주입 (thread-safe).
    // IO 스레드/워커에서 호출.
    // 주입된 요청은 다음 프레임 빌드 시 처리된다.
    virtual void SubmitDynamicTask(DynamicTaskRequest request) noexcept = 0;

    // 아웃바운드 IO 완료 → Suspended 노드 재개 (thread-safe).
    // 내부에서 ExecutorIdleCoordinator::Wake()를 호출하여 대기 워커를 깨운다.
    virtual void PushCompletion(CompletionEntry entry) noexcept = 0;

    // 외부 IO backend가 작업을 게시했음을 알린다.
    // executor가 idle 상태라면 worker 한 명을 깨워 처리하게 한다.
    // IO 종류(네트워크, DB 등)에 무관하게 모든 backend가 이 메서드만 호출한다.
    // Step 4 이후 내부에서 ExecutorIdleCoordinator::Wake()로 위임된다.
    virtual void WakeForExternalIO() noexcept = 0;

    virtual ~IExecutorIOSink() = default;
};
