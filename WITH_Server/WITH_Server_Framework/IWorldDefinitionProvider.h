#pragma once

class ExecutionSourceRegistry;
class WorldExecutionModelRegistry;
class WorldTransferProfileRegistry;
class WorldRegistry;

class IWorldDefinitionProvider {
public:
    virtual ~IWorldDefinitionProvider() = default;

    virtual bool RegisterExecutionSources(
        ExecutionSourceRegistry& sourceRegistry) const = 0;

    virtual bool RegisterExecutionModels(
        const ExecutionSourceRegistry& sourceRegistry,
        WorldExecutionModelRegistry& executionModelRegistry) const = 0;

    virtual bool RegisterTransferProfiles(
        WorldTransferProfileRegistry& transferProfileRegistry) const
    {
        (void)transferProfileRegistry;
        return true;
    }

    virtual bool RegisterWorldDefs(
        WorldRegistry& worldRegistry) const = 0;
};
