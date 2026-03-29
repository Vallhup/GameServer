#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ExecutionCoreTypes.h"

struct ExecutionSourceDesc
{
    ExecToken token{ InvalidExecToken };

    ExecPhase phase{ ExecPhase::Simulate };
    ExecLane lane{ ExecLane::Parallel };
    ExecNodeKind kind{ ExecNodeKind::None };
    uint32_t flags{ ExecNodeFlag_None };

    std::string debugName;

    // TODO
    //
    // 1. callback pointer
    // 2. bound runtime function
    // 3. ECS write/read footprint
    // 4. metadata-based scheduling contract

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return token != InvalidExecToken &&
            kind != ExecNodeKind::None;
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

    void Register(const ExecutionSourceDesc& desc)
    {
        _sources.push_back(desc);
    }

    void Clear()
    {
        _sources.clear();
    }

private:
    std::vector<ExecutionSourceDesc> _sources;
};