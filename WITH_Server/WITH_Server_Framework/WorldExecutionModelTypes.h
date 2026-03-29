#pragma once

#include <cstdint>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "WorldFrameSelectionTypes.h"

struct ExecutionDependencyEdge
{
    ExecToken fromToken{ InvalidExecToken };
    ExecToken toToken{ InvalidExecToken };

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            fromToken != InvalidExecToken &&
            toToken != InvalidExecToken;
    }
};

// 1차 구현 validation 규칙
//
// 1. model 전체 source 집합에서 token 중복 금지
// 2. explicit edge의 from/to token은 반드시 model 내부 source여야 함
// 3. self-edge 금지
// 4. source descriptor.phase 와 model declared phase는 일치해야 함
// 5. 뒤 phase -> 앞 phase 역행 edge 금지
//    허용 순서: Simulate <= Commit <= LifecycleFlush <= Reconcile
// 6. 같은 phase 내부 cycle 금지
// 7. 앞 phase -> 뒤 phase edge는 허용
//    단, serial phase ordering / build ordering에만 반영하고
//    runtime remainingDeps에는 반영하지 않음

struct WorldExecutionModel
{
    WorldExecutionModelKey key{ InvalidWorldExecutionModelKey };

    std::vector<ExecToken> simulateSources;
    std::vector<ExecToken> commitSources;
    std::vector<ExecToken> lifecycleFlushSources;
    std::vector<ExecToken> reconcileSources;

    std::vector<ExecutionDependencyEdge> explicitEdges;

    // TODO
    //
    // 1. per-source condition rules
    // 2. source groups
    // 3. world state predicates

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return key != InvalidWorldExecutionModelKey;
    }

    [[nodiscard]]
    bool ContainSourceToken(ExecToken token) const noexcept
    {
        return TryGetDeclaredPhase(token) != ExecPhase::None;
    }

    [[nodiscard]]
    ExecPhase TryGetDeclaredPhase(ExecToken token) const noexcept
    {
        if (token == InvalidExecToken)
            return ExecPhase::None;

        for (ExecToken t : simulateSources)
        {
            if (t == token)
                return ExecPhase::Simulate;
        }

        for (ExecToken t : commitSources)
        {
            if (t == token)
                return ExecPhase::Commit;
        }

        for (ExecToken t : lifecycleFlushSources)
        {
            if (t == token)
                return ExecPhase::LifecycleFlush;
        }

        for (ExecToken t : reconcileSources)
        {
            if (t == token)
                return ExecPhase::Reconcile;
        }

        return ExecPhase::None;
    }

    [[nodiscard]]
    uint32_t GetSourceCount() const noexcept
    {
        return static_cast<uint32_t>(
            simulateSources.size() +
            commitSources.size() +
            lifecycleFlushSources.size() +
            reconcileSources.size());
    }
};

class WorldExecutionModelRegistry {
public:
    [[nodiscard]]
    const WorldExecutionModel* TryGet(WorldExecutionModelKey key) const noexcept
    {
        for (const WorldExecutionModel& model : _models)
        {
            if (model.key == key)
                return &model;
        }
        return nullptr;
    }

    void Register(const WorldExecutionModel& model)
    {
        _models.push_back(model);
    }

    void Clear()
    {
        _models.clear();
    }

private:
    std::vector<WorldExecutionModel> _models;
};