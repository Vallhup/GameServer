#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionRuntimeTypes.h"
#include "DynamicTaskTypes.h"
#include "WorldFrameSelectionTypes.h"
#include "WorldId.h"

class WorldManager;
class WorldRegistry;
class WorldRuntime;
class WorldExecutionModelRegistry;
class ExecutionSourceRegistry;
class ExecutionGraphBuilder;
struct ExecutionGraphBuildPolicy;
class TaskExecutor;
class ExecutionOps;
class DynamicTaskTypeRegistry;
class DynamicTaskScheduler;

enum class WorldSchedulerFailureReason : uint8_t
{
    None,
    RuntimeResolveFailed,
    BeginFrameFailed,
    GraphBuildFailed,
    ExecutorFailed,
    InternalInvariant,
};

struct WorldSchedulerConfig
{
    uint32_t maxSelectedWorldsPerFrame{ 0 };
};

struct WorldSchedulerFrameParams
{
    uint64_t frameIndex{ 0 };
    double nowSec{ 0.0 };
    double dtSec{ 0.0 };
};

struct WorldSchedulerFrameResult
{
    bool success{ false };
    bool graphBuilt{ false };
    bool executed{ false };

    uint32_t selectedWorldCount{ 0 };
    WorldSchedulerFailureReason failureReason{ WorldSchedulerFailureReason::None };
};

class WorldScheduler final
{
public:
    WorldScheduler(
        WorldManager& worldManager,
        WorldRegistry& worldRegistry,
        WorldExecutionModelRegistry& executionModelRegistry,
        ExecutionSourceRegistry& executionSourceRegistry,
        ExecutionGraphBuilder& graphBuilder,
        const ExecutionGraphBuildPolicy& buildPolicy,
        TaskExecutor& executor,
        ExecutionOps& executionOps,
        WorldSchedulerConfig config = {},
        DynamicTaskTypeRegistry* dynamicTaskTypeRegistry = nullptr,
        DynamicTaskScheduler* dynamicTaskScheduler = nullptr
    );

    bool RunFrame(
        const WorldSchedulerFrameParams& params,
        WorldSchedulerFrameResult& outResult);

    const WorldFrameSelectionSet& GetLastSelectionSet() const noexcept;
    const BuildResult& GetLastBuildResult() const noexcept;

    void Clear() noexcept;

private:
    struct FrameScratch
    {
        WorldFrameSelectionSet selections;
        BuildResult buildResult;

        std::vector<WorldRuntime*> runtimeByScope;
        std::vector<WorldId> worldIdByScope;

        std::unique_ptr<ExecNodeRuntime[]> nodeBacking;
        std::unique_ptr<ExecScopeRuntime[]> scopeBacking;

        // dynamic task freeze: 프레임 경계에서 채워지고 Build()에 전달된다.
        DynamicTaskFrozenBatch dynamicBatch;

        FrameExecContext frameExec{};
        ExecRuntimeState execRuntime{};

        void Reset() noexcept;
    };

private:
    bool BuildSelectionSet(
        const WorldSchedulerFrameParams& params,
        WorldFrameSelectionSet& outSelections);

    bool ResolveSelectedRuntimes(
        const WorldFrameSelectionSet& selections,
        std::vector<WorldRuntime*>& outRuntimeByScope,
        std::vector<WorldId>& outWorldIdByScope);

    bool BeginSelectedFrames(
        const WorldSchedulerFrameParams& params,
        std::span<WorldRuntime*> runtimeByScope);

    bool BuildFrameGraph(
        const WorldFrameSelectionSet& selections,
        const DynamicTaskFrozenBatch& dynamicBatch,
        BuildResult& outBuildResult);

    bool PrepareExecutionContexts(
        const FrameTaskGraph& graph,
        DynamicTaskFrameTable& dynamicTaskFrameTable,
        std::vector<WorldRuntime*>& runtimeByScope,
        std::vector<WorldId>& worldIdByScope,
        FrameExecContext& outFrameExec,
        ExecRuntimeState& outExecRuntime);

    // 프레임 경계에서 모든 선택된 world의 pending queue와 Network Inbound 요청을 drain하고
    // 결정론적 정렬 후 outBatch에 채운다.
    void FreezeDynamicTaskRequests(
        std::span<WorldRuntime*> runtimeByScope,
        uint64_t frameIndex,
        DynamicTaskFrozenBatch& outBatch);

private:
    WorldManager& _worldManager;
    WorldRegistry& _worldRegistry;

    WorldExecutionModelRegistry& _executionModelRegistry;
    ExecutionSourceRegistry& _executionSourceRegistry;
    ExecutionGraphBuilder& _graphBuilder;
    const ExecutionGraphBuildPolicy& _buildPolicy;

    TaskExecutor& _executor;
    ExecutionOps& _executionOps;

    // optional — nullptr이면 Dynamic Task 기능이 비활성화된다.
    DynamicTaskTypeRegistry* _dynamicTaskTypeRegistry{ nullptr };
    DynamicTaskScheduler* _dynamicTaskScheduler{ nullptr };

    WorldSchedulerConfig _config;
    FrameScratch _scratch;
};
