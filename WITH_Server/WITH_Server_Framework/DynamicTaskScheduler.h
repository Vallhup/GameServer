#pragma once

#include <vector>
#include <concurrent_queue.h>

#include "DynamicTaskTypes.h"

// 네트워크 모듈 → 다음 프레임 DynamicTask 요청 누적기.
//
// Submit()은 임의 스레드(네트워크 콜백 등)에서 thread-safe하게 호출된다.
// DrainInto()는 프레임 경계(WorldScheduler::FreezeDynamicTaskRequests 호출 시)에서
// 단독으로 호출한다.
//
// 소유권: 애플리케이션 계층(ServerCore 등)이 소유하고, TaskExecutor와 WorldScheduler에 주입한다.
class DynamicTaskScheduler {
public:
    // thread-safe. 네트워크 IO 스레드 / 워커 스레드에서 호출 가능하다.
    void Submit(DynamicTaskRequest request) noexcept;

    // 프레임 경계(단일 스레드)에서 호출한다.
    // pending 전체를 out에 이동 추가하고 큐를 비운다.
    // WorldScheduler::FreezeDynamicTaskRequests가 outBatch.requests를 대상으로 호출한다.
    void DrainInto(std::vector<DynamicTaskRequest>& out);

private:
    concurrency::concurrent_queue<DynamicTaskRequest> _pending;
};
