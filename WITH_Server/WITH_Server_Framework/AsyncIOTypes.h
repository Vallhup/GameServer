#pragma once

#include <atomic>
#include <cstdint>

#include "ExecutionCoreTypes.h"

// IO 완료 결과 — 범용 슬롯. 실제 데이터 해석은 호출부(continuation)에서 수행.
struct IoResult
{
    bool        success{ false };
    const void* data{ nullptr };
    uint32_t    size{ 0 };
};

using ExecFn = ExecCallResult(*)(struct NodeExecContext&);

// 아웃바운드 비동기 IO 핸들.
// alignas(64): 서로 다른 워커 스레드의 false sharing 방지.
struct alignas(64) IoHandle
{
    std::atomic<uint32_t>   epoch;          // TriggerSweep 시 증가. 구 epoch 완료는 무시.
    std::atomic<ExecNodeId> activeNode;     // Suspend 노드 ID. Sweep 후 continuation 노드 ID.
    std::atomic<uint32_t>   refCount;       // 0 → ObjectPool 반환. 초기값 2 (노드 ref + 콜백 ref).
    ExecFn                  continuationFn{ nullptr };  // Sweep 시 다음 프레임 DynamicTask ExecFn.
    IoResult                pendingResult{};            // ProcessCompletions() 또는 Sweep 시 설정.
};

struct CompletionEntry
{
    IoHandle* handle;
    IoResult  result;
};
