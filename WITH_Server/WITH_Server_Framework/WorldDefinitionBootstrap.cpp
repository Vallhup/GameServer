#include "pch.h"
#include "WorldDefinitionBootstrap.h"

#include "IWorldDefinitionProvider.h"
#include "ExecutionSourceTypes.h"
#include "WorldExecutionModelTypes.h"
#include "WorldTransferProfileRegistry.h"
#include "WorldRegistry.h"

bool BootstrapWorldDefinitions(
    const IWorldDefinitionProvider& provider,
    ExecutionSourceRegistry& sourceRegistry,
    WorldExecutionModelRegistry& executionModelRegistry,
    WorldTransferProfileRegistry& transferProfileRegistry,
    WorldRegistry& worldRegistry)
{
    if (!provider.RegisterExecutionSources(sourceRegistry))
        return false;

    if (!provider.RegisterExecutionModels(sourceRegistry, executionModelRegistry))
        return false;

    if (!provider.RegisterTransferProfiles(transferProfileRegistry))
        return false;

    if (!provider.RegisterWorldDefs(worldRegistry))
        return false;

    return true;
}
