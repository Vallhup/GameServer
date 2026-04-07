#pragma once

class IWorldDefinitionProvider;
class ExecutionSourceRegistry;
class WorldExecutionModelRegistry;
class WorldRegistry;

bool BootstrapWorldDefinitions(
    const IWorldDefinitionProvider& provider,
    ExecutionSourceRegistry& sourceRegistry,
    WorldExecutionModelRegistry& executionModelRegistry,
    WorldRegistry& worldRegistry);
