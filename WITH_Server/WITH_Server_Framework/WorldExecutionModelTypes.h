#pragma once

#include <cstdint>
#include <vector>

#include "ExecutionCoreTypes.h"
#include "WorldFrameSelectionTypes.h"

struct WorldExecutionModel
{
    WorldExecutionModelKey key{ InvalidWorldExecutionModelKey };

    std::vector<ExecToken> simulateSources;
    std::vector<ExecToken> commitSources;
    std::vector<ExecToken> lifecycleFlushSources;
    std::vector<ExecToken> reconcileSources;

    // TODO
    //
    // 1. explicit dependency edges
    // 2. per-source condition rules
    // 3. source groups
    // 4. world state predicates

    [[nodiscard]]
    bool IsValid() const noexcept
    {
        return key != InvalidWorldExecutionModelKey;
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