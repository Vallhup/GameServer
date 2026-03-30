#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionRuntimeTypes.h"
#include "ExecutionSourceTypes.h"
#include "ThreadPool.h"

class TaskExecutor final {
public:
    explicit TaskExecutor(uint32_t workerCount = 0);
    ~TaskExecutor();

    TaskExecutor(const TaskExecutor&) = delete;
    TaskExecutor& operator=(const TaskExecutor&) = delete;

    [[nodiscard]]
    bool Initialize(uint32_t workerCount = 0);

    void Shutdown() noexcept;

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

private:
    struct FrameBinding
    {
        FrameExecContext* frame{ nullptr };
        ExecRuntimeState* runtime{ nullptr };
        const ExecutionSourceRegistry* sources{ nullptr };
    };

private:
    static bool WorkerPumpEntry(void* ctx);

    [[nodiscard]]
    bool WorkerPump();

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

    void FinalizeScopeClosures() noexcept;

    void WaitForSimulateDone();

    [[nodiscard]]
    bool TryDequeueReadyNode(ExecNodeId& outNodeId);

    void EnqueueReadyNode(ExecNodeId nodeId);

    void DispatchNode(ExecNodeId nodeId);

    void ExecuteNode(ExecNodeId nodeId);

    [[nodiscard]]
    ExecCallResult InvokeNode(
        const ExecNodeRecord& node,
        ExecNodeId nodeId
    ) const;

    void ResolveSuccessors(ExecNodeId completedNodeId);

    void CompleteNodeTerminal(
        ExecNodeId nodeId,
        ExecNodeState terminalState
    ) noexcept;

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

    FrameBinding _binding{};

    std::mutex _readyMtx;
    std::deque<ExecNodeId> _readyQueue;

    std::mutex _progressMtx;
    std::condition_variable _progressCv;

    std::atomic<bool> _frameBound{ false };
};