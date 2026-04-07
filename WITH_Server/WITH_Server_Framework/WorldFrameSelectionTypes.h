#pragma once

#include <cstdint>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "WorldContentIds.h"

using WorldBinding = uint64_t;

constexpr WorldBinding InvalidWorldBinding = 0;

struct WorldFrameSelection
{
    ExecScopeId scopeId{ InvalidExecScopeId };
    WorldBinding worldBinding{ InvalidWorldBinding };
    WorldExecutionModelKey modelKey{ InvalidWorldExecutionModelKey };

    // TODO
    //
    // 1. dt
    // 2. fixedStepCount
    // 3. catchUpCount
    // 4. execution priority
    // 5. world stage

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return scopeId != InvalidExecScopeId &&
            worldBinding != InvalidWorldBinding &&
            modelKey != InvalidWorldExecutionModelKey;
    }
};

struct WorldFrameSelectionSet
{
    std::vector<WorldFrameSelection> selections;

    [[nodiscard]]
    bool IsEmpty() const noexcept
    {
        return selections.empty();
    }

    [[nodiscard]]
    uint32_t GetCount() const noexcept
    {
        return static_cast<uint32_t>(selections.size());
    }

    [[nodiscard]]
    const WorldFrameSelection* TryGet(ExecScopeId scopeId) const noexcept
    {
        for (const WorldFrameSelection& s : selections)
        {
            if (s.scopeId == scopeId)
                return &s;
        }
        return nullptr;
    }

    void Clear()
    {
        selections.clear();
    }
};
