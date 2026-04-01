#pragma once

#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"

class ServerWorldBootstrapFactory final : public IWorldInstanceFactory {
public:
	std::unique_ptr<IWorldInstanceImpl> Create(const WorldDef& def) override;
};

class ServerWorldBootstrapDefinitionProvider final
	: public IWorldDefinitionProvider {
public:
	bool RegisterExecutionSources(
		ExecutionSourceRegistry& sourceRegistry) const override;

	bool RegisterExecutionModels(
		const ExecutionSourceRegistry& sourceRegistry,
		WorldExecutionModelRegistry& executionModelRegistry) const override;

	bool RegisterWorldDefs(
		WorldRegistry& worldRegistry) const override;
};
