#pragma once

class IWorldDefinitionProvider;
class ExecutionSourceRegistry;
class WorldExecutionModelRegistry;
class WorldTransferProfileRegistry;
class WorldRegistry;

bool BootstrapWorldDefinitions(
    const IWorldDefinitionProvider& provider,
    ExecutionSourceRegistry& sourceRegistry,
    WorldExecutionModelRegistry& executionModelRegistry,
    WorldTransferProfileRegistry& transferProfileRegistry,
    WorldRegistry& worldRegistry);
