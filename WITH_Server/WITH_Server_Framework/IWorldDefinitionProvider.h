#pragma once

class ExecutionSourceRegistry;
class WorldExecutionModelRegistry;
class WorldRegistry;

class IWorldDefinitionProvider {
public:
    virtual ~IWorldDefinitionProvider() = default;

    virtual bool RegisterExecutionSources(
        ExecutionSourceRegistry& sourceRegistry) const = 0;

    virtual bool RegisterExecutionModels(
        const ExecutionSourceRegistry& sourceRegistry,
        WorldExecutionModelRegistry& executionModelRegistry) const = 0;

    virtual bool RegisterWorldDefs(
        WorldRegistry& worldRegistry) const = 0;
};
