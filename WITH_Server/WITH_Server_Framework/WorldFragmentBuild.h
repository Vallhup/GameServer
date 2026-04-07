#pragma once

#include <cstdint>
#include <vector>

#include "ExecutionGraphTypes.h"
#include "WorldFrameSelectionTypes.h"

struct LocalFragmentEdge
{
    uint32_t fromLocalIndex{ 0 };
    uint32_t toLocalIndex{ 0 };
};

struct WorldFragmentBuild
{
    ExecScopeId scopeId{ InvalidExecScopeId };
    WorldBinding worldBinding{ InvalidWorldBinding };
    WorldExecutionModelKey modelKey{ InvalidWorldExecutionModelKey };

    // local node table for this world fragment.
    // id/pred/succ range are finalized during AssembleFrameGraph().
    std::vector<ExecNodeRecord> localNodes;

    // local DAG edges expressed in local node indices.
    std::vector<LocalFragmentEdge> localEdges;

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            scopeId != InvalidExecScopeId &&
            worldBinding != InvalidWorldBinding &&
            modelKey != InvalidWorldExecutionModelKey;
    }

    [[nodiscard]]
    bool IsValidLocalIndex(uint32_t index) const noexcept
    {
        return index < static_cast<uint32_t>(localNodes.size());
    }

    void Clear() noexcept
    {
        scopeId = InvalidExecScopeId;
        worldBinding = InvalidWorldBinding;
        modelKey = InvalidWorldExecutionModelKey;
        localNodes.clear();
        localEdges.clear();
    }
};