#pragma once

#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"

class AnimationRegistry;

class ServerWorldBootstrapFactory final : public IWorldInstanceFactory {
public:
	void SetAnimationRegistry(
		const AnimationRegistry* animationRegistry) noexcept;

	std::unique_ptr<IWorldInstanceImpl> Create(const WorldDef& def) override;

private:
	const AnimationRegistry* _animationRegistry{ nullptr };
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
