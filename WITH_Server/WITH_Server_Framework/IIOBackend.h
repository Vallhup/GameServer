#pragma once

#include <cstdint>

// Executor → 외부 IO backend 인터페이스.
// IO 종류(네트워크, DB, File, Timer 등)에 중립.
//
// 설계 원칙:
//   - sleep 책임은 갖지 않는다. ExecutorIdleCoordinator(Step 4)가 단독 책임.
//   - backend의 책임은 "이미 도착한 completion을 비차단으로 빼내기"와
//     "디버그 식별자 제공" 뿐이다.
//   - IO 종류 고유 동작(Send, Disconnect 등)은 sub-interface가 추가한다.
//   - 새 completion이 도착하면 backend의 IO 스레드에서
//     IExecutorIOSink::WakeForExternalIO()를 호출하여 idle worker를 깨운다.
struct IIOBackend
{
    // 이미 도착한 completion을 가능한 만큼 비차단으로 빼낸다.
    // - 작업이 없으면 즉시 false 반환.
    // - 작업이 하나라도 있으면 처리 후 true 반환.
    // - blocking 금지 — sleep 책임은 ExecutorIdleCoordinator에 있다.
    [[nodiscard]]
    virtual bool DrainCompletions(uint32_t workerIdx) noexcept = 0;

    // 디버그/진단용 식별자. 예: "Network", "DB", "File".
    [[nodiscard]]
    virtual const char* DebugName() const noexcept = 0;

    virtual ~IIOBackend() = default;
};
