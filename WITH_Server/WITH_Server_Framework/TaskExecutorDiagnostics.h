#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionSourceTypes.h"

struct TaskExecutorDiagnosticsConfig
{
    bool enabled{ false };
    bool collectNodeTimings{ false };
    bool logFrameSummary{ false };
    bool writeCsv{ false };
    uint32_t sampleEveryNFrames{ 1 };
    std::string summaryCsvPath;
    std::string nodeCsvPath;
};

struct TaskExecutorNodeDiagnostic
{
    ExecNodeId nodeId{ InvalidExecNodeId };
    ExecScopeId scopeId{ InvalidExecScopeId };
    ExecToken sourceToken{ InvalidExecToken };
    ExecPhase phase{ ExecPhase::None };
    ExecNodeKind kind{ ExecNodeKind::None };
    uint32_t flags{ ExecNodeFlag_None };
    ExecCallResult result{ ExecCallResult::Failed };
    uint64_t threadHash{ 0 };
    uint64_t startNs{ 0 };
    uint64_t endNs{ 0 };
    uint64_t durationNs{ 0 };
    std::string debugName;
};

struct TaskExecutorFrameDiagnostics
{
    bool valid{ false };
    uint64_t frameOrdinal{ 0 };
    uint32_t workerCount{ 0 };
    uint32_t nodeCount{ 0 };
    uint32_t simulateNodeCount{ 0 };
    uint32_t scopeCount{ 0 };
    uint32_t edgeCount{ 0 };
    uint32_t executedNodes{ 0 };
    uint32_t maxConcurrency{ 0 };
    uint32_t distinctThreads{ 0 };
    uint64_t wallNs{ 0 };
    uint64_t sumNodeNs{ 0 };
    uint64_t minNodeNs{ 0 };
    uint64_t maxNodeNs{ 0 };
    uint64_t idealNs{ 0 };
    int64_t overheadVsIdealNs{ 0 };
    double workerEfficiency{ 0.0 };
    std::vector<TaskExecutorNodeDiagnostic> nodes;

    void Clear() noexcept;
};

class TaskExecutorDiagnosticsRecorder final
{
public:
    TaskExecutorDiagnosticsRecorder();
    ~TaskExecutorDiagnosticsRecorder();

    TaskExecutorDiagnosticsRecorder(const TaskExecutorDiagnosticsRecorder&) = delete;
    TaskExecutorDiagnosticsRecorder& operator=(const TaskExecutorDiagnosticsRecorder&) = delete;

    void Configure(TaskExecutorDiagnosticsConfig config);

    void BeginFrame(
        const FrameTaskGraph& graph,
        uint32_t workerCount,
        uint64_t frameOrdinal);

    void EndFrame(
        const FrameTaskGraph& graph,
        const ExecutionSourceRegistry& sourceRegistry);

    void BeginNode(const ExecNodeRecord& node, ExecNodeId nodeId) noexcept;
    void EndNode(
        const ExecNodeRecord& node,
        ExecNodeId nodeId,
        ExecCallResult result) noexcept;

    [[nodiscard]]
    const TaskExecutorDiagnosticsConfig& GetConfig() const noexcept
    {
        return _config;
    }

    [[nodiscard]]
    const TaskExecutorFrameDiagnostics& GetLastFrame() const noexcept
    {
        return _lastFrame;
    }

private:
    struct NodeTimingSlot;
    static constexpr uint64_t kNsPerUs = 1000;
    static constexpr const char* kLogCategory = "ExecutorPerf";

    [[nodiscard]]
    static uint64_t NowNs() noexcept;
    [[nodiscard]]
    static double ToUs(uint64_t ns) noexcept;
    [[nodiscard]]
    static double ToUsSigned(int64_t ns) noexcept;
    [[nodiscard]]
    static const char* PhaseName(ExecPhase phase) noexcept;
    [[nodiscard]]
    static const char* KindName(ExecNodeKind kind) noexcept;
    [[nodiscard]]
    static const char* ResultName(ExecCallResult result) noexcept;

    static void UpdateAtomicMax(std::atomic<uint32_t>& target, uint32_t value) noexcept;

    void EnsureSlotCapacity(size_t nodeCount);
    void ResetSlots(size_t nodeCount) noexcept;
    void BuildFrameSnapshot(
        const FrameTaskGraph& graph,
        const ExecutionSourceRegistry& sourceRegistry,
        uint64_t frameEndNs);
    void EmitFrameSummary() const;
    void WriteCsvFiles();
    void WriteSummaryCsv();
    void WriteNodeCsv();

private:
    TaskExecutorDiagnosticsConfig _config{};
    TaskExecutorFrameDiagnostics _lastFrame{};
    std::unique_ptr<NodeTimingSlot[]> _slots;
    size_t _slotCapacity{ 0 };
    size_t _activeSlotCount{ 0 };
    uint64_t _frameStartNs{ 0 };
    uint32_t _workerCount{ 0 };
    uint64_t _frameOrdinal{ 0 };
    bool _recording{ false };
    bool _summaryHeaderWritten{ false };
    bool _nodeHeaderWritten{ false };
    std::atomic<uint32_t> _activeNodes{ 0 };
    std::atomic<uint32_t> _maxActiveNodes{ 0 };
    std::atomic<uint32_t> _executedNodes{ 0 };
};
