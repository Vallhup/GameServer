#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionRuntimeTypes.h"
#include "ExecutionSourceTypes.h"
#include "AsyncIOTypes.h"
#include "INetworkBackend.h"
#include "IExecutorIOSink.h"
#include "ObjectPool.hpp"
#include <concurrent_queue.h>

#include "LFWSDeque.h"
#include "TaskExecutorDiagnostics.h"
#include "ThreadPool.h"

class DynamicTaskScheduler;

class TaskExecutor final
    : public IIoHandleProvider
    , public IExecutorIOSink
{
public:
    explicit TaskExecutor(uint32_t workerCount = 0);
    ~TaskExecutor();

    TaskExecutor(const TaskExecutor&) = delete;
    TaskExecutor& operator=(const TaskExecutor&) = delete;

    [[nodiscard]]
    bool Initialize(uint32_t workerCount = 0);

    void Shutdown() noexcept;

    // 네트워크 백엔드 등록. PushCompletion → WakeWorker 경로에 사용.
    // ExecuteFrame 호출 전에 설정해야 한다.
    void SetNetworkBackend(INetworkBackend* backend) noexcept;

    // 인바운드 DynamicTask 스케줄러 등록.
    // SubmitDynamicTask() 호출 전에 설정해야 한다.
    void SetDynamicTaskScheduler(DynamicTaskScheduler* scheduler) noexcept;

    [[nodiscard]]
    bool IsInitialized() const noexcept
    {
        return _initialized.load(std::memory_order_acquire);
    }

    [[nodiscard]]
    bool ExecuteFrame(
        FrameExecContext& frameCtx,
        ExecRuntimeState& runtime,
        const ExecutionSourceRegistry& sourceRegistry
    );

    void SetDiagnosticsConfig(TaskExecutorDiagnosticsConfig config);

    [[nodiscard]]
    const TaskExecutorFrameDiagnostics& GetLastFrameDiagnostics() const noexcept;

private:
    // 워커 deque 용량. 프레임 당 단일 워커가 누적할 수 있는 최대 노드 수.
    // 초과 시 EnqueueReadyNode가 자동으로 _injectQueue로 fallback한다.
    static constexpr int64_t kWorkerDequeCapacity = 1024;

    // workerIdx 파라미터가 없는 컨텍스트(메인 스레드 등)에서 사용하는 sentinel.
    static constexpr uint32_t kNoWorkerIdx = ~0u;

    struct FrameBinding
    {
        FrameExecContext* frame{ nullptr };
        ExecRuntimeState* runtime{ nullptr };
        const ExecutionSourceRegistry* sources{ nullptr };
    };

    struct ExecutingNodeGuard
    {
        explicit ExecutingNodeGuard(TaskExecutor& executor) noexcept;
        ~ExecutingNodeGuard();

        ExecutingNodeGuard(const ExecutingNodeGuard&) = delete;
        ExecutingNodeGuard& operator=(const ExecutingNodeGuard&) = delete;

        TaskExecutor& executor;
    };

private:
    static bool WorkerPumpEntry(void* ctx, uint32_t workerIdx);

    [[nodiscard]]
    bool WorkerPump(uint32_t workerIdx);

    [[nodiscard]]
    bool ValidateFrameInputs(
        const FrameExecContext& frameCtx,
        const ExecRuntimeState& runtime
    ) const noexcept;

    void BindFrame(
        FrameExecContext& frameCtx,
        ExecRuntimeState& runtime,
        const ExecutionSourceRegistry& sourceRegistry
    ) noexcept;

    void UnbindFrame() noexcept;

    void InitializeRuntimeForFrame();

    void SeedInitialSimulateNodes();

    void RunSerialPhase(
        const ExecRange& range,
        ExecPhase expectedPhase
    );

    void RunScopeSerialPhase(
        const ExecRange& range,
        ExecPhase expectedPhase
    );

    void FinalizeScopeClosures() noexcept;

    void WaitForSimulateDone();

    // 메인 스레드 전용 큐에 있는 노드를 모두 실행한다.
    // WaitForSimulateDone 루프 내부와 그 직후에 호출된다.
    void DrainMainThreadQueue();

    // workerIdx가 kNoWorkerIdx이면 공유 큐에 삽입한다.
    // 유효한 workerIdx이면 해당 워커 deque에 TryPush를 시도하고,
    // deque가 가득 찬 경우 공유 큐로 fallback한다.
    void EnqueueReadyNode(ExecNodeId nodeId, uint32_t workerIdx);

    void DispatchNode(ExecNodeId nodeId, uint32_t workerIdx);

    void ExecuteNode(ExecNodeId nodeId, uint32_t workerIdx);

    [[nodiscard]]
    ExecCallResult InvokeNode(
        const ExecNodeRecord& node,
        ExecNodeId nodeId
    );

    void ResolveSuccessors(ExecNodeId completedNodeId, uint32_t workerIdx);

    void CompleteNodeTerminal(
        ExecNodeId nodeId,
        ExecNodeState terminalState
    ) noexcept;

    void TryPublishSimulateDone() noexcept;

    void MarkScopeFailedAndCancelRequested(ExecScopeId scopeId) noexcept;

    void MarkScopeCanceled(ExecScopeId scopeId) noexcept;

    [[nodiscard]]
    ExecNodeState SelectCancelTerminalState(
        const ExecNodeRecord& node
    ) const noexcept;

    [[nodiscard]]
    bool TryTransitionNode(
        ExecNodeRuntime& nodeRt,
        ExecNodeState expected,
        ExecNodeState desired
    ) const noexcept;

    void NotifyWork() noexcept;
    void NotifyAllWorkers() noexcept;
    void NotifyProgress() noexcept;

    // IIoHandleProvider 구현
    IoHandle* Acquire(ExecNodeId nodeId, uint32_t epoch) noexcept override;

    // IExecutorIOSink 구현
    void SubmitDynamicTask(DynamicTaskRequest request) noexcept override;
    void PushCompletion(CompletionEntry entry) noexcept override;
    void WakeForNetworkIO() noexcept override;

    // AsyncIO 내부
    void ProcessCompletions(uint32_t workerIdx);
    void TriggerSweep() noexcept;
    void ResumeNode(ExecNodeId nodeId, uint32_t workerIdx);

    [[nodiscard]]
    const FrameTaskGraph& Graph() const noexcept
    {
        return *_binding.frame->graph;
    }

    [[nodiscard]]
    FrameExecContext& Frame() noexcept
    {
        return *_binding.frame;
    }

    [[nodiscard]]
    ExecRuntimeState& Runtime() noexcept
    {
        return *_binding.runtime;
    }

    [[nodiscard]]
    const ExecutionSourceRegistry& Sources() const noexcept
    {
        return *_binding.sources;
    }

private:
    ThreadPool _pool;
    std::atomic<bool> _initialized{ false };
    TaskExecutorDiagnosticsRecorder _diagnostics;
    uint64_t _diagnosticsFrameOrdinal{ 0 };

    FrameBinding _binding{};

    // 워커별 Lock-Free Work-Stealing Deque.
    // 인덱스 i의 deque는 워커 i만 Push/TryPop하고,
    // 다른 워커는 TrySteal만 호출한다.
    // Initialize 시 workerCount 크기로 할당되며 프레임마다 Reset된다.
    std::vector<std::unique_ptr<LFWSDeque<ExecNodeId, kWorkerDequeCapacity>>> _workerDeques;

    // 초기 시드(메인 스레드) 및 워커 deque 가득 찬 경우의 lock-free fallback 큐.
    // 다수 생산자(메인 스레드 + 임의 워커) / 다수 소비자(모든 워커) → MPMC.
    // VS PPL concurrent_queue: lock-free, unbounded.
    // WorkerPump에서는 TrySteal 실패 후 마지막 경로로 확인한다.
    concurrency::concurrent_queue<ExecNodeId> _injectQueue;

    // MainThreadOnly 노드 전용 큐. 메인 스레드만 소비하며 _mainQueueMtx로 보호한다.
    std::mutex             _mainQueueMtx;
    std::deque<ExecNodeId> _mainThreadReadyQueue;

    std::mutex _progressMtx;
    std::condition_variable _progressCv;

    std::atomic<bool> _frameBound{ false };
    std::atomic<uint64_t> _dynamicTaskSubmissionSequence{ 1 };

    // TEMP_TASKEXECUTOR_DEBUG: frame-boundary/queue race diagnostics. Remove after root cause is fixed.
    std::atomic<uint64_t> _debugNextFrameId{ 1 };
    std::atomic<uint64_t> _debugCurrentFrameId{ 0 };
    std::atomic<uint32_t> _debugActiveWorkerPumps{ 0 };
    std::atomic<uint32_t> _activeExecutingNodes{ 0 };

    // AsyncIO
    FrameState _frameState;
    concurrency::concurrent_queue<CompletionEntry> _completionQueue;
    ObjectPool<IoHandle, 1024, OverflowPolicy_Nullptr<IoHandle>> _ioHandlePool;
    INetworkBackend* _networkBackend{ nullptr };
    DynamicTaskScheduler* _dynamicTaskScheduler{ nullptr };
};
