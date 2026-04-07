#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ExecutionCoreTypes.h"

struct ExecutionSourceDesc
{
    ExecToken token{ InvalidExecToken };

    ExecPhase phase{ ExecPhase::None };
    ExecLane lane{ ExecLane::None };
    ExecNodeKind kind{ ExecNodeKind::None };
    uint32_t flags{ ExecNodeFlag_None };

    ExecFn fn{ nullptr };

    std::string debugName;

    // TODO
    //
    // 1. bound runtime function
    // 2. ECS write/read footprint
    // 3. metadata-based scheduling contract

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return
            token != InvalidExecToken &&
            kind != ExecNodeKind::None &&
            fn != nullptr;
    }
};

class ExecutionSourceRegistry {
public:
    [[nodiscard]]
    const ExecutionSourceDesc* TryGet(ExecToken token) const noexcept
    {
        for (const ExecutionSourceDesc& desc : _sources)
        {
            if (desc.token == token)
                return &desc;
        }
        return nullptr;
    }

    [[nodiscard]]
    bool Register(const ExecutionSourceDesc& desc)
    {
        if (!desc.IsValid())
            return false;

        if (TryGet(desc.token) != nullptr)
            return false;

        _sources.push_back(desc);
        return true;
    }

    void Clear()
    {
        _sources.clear();
    }

private:
    // TODO: 선형 탐색 병목 시 unordered_map 기반으로 수정
    std::vector<ExecutionSourceDesc> _sources;
};