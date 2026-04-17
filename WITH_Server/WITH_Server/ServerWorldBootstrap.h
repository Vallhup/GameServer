#pragma once

#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"
#include "WorldId.h"

class AnimationRegistry;
class FrameworkRuntime;
class ServerApp;

class ServerWorldBootstrapFactory final : public IWorldInstanceFactory {
public:
	void SetAnimationRegistry(
		const AnimationRegistry* animationRegistry) noexcept;
	void SetFramework(FrameworkRuntime* framework) noexcept;
	void SetBootstrapWorldId(const WorldId* worldId) noexcept;

	std::unique_ptr<IWorldInstanceImpl> Create(const WorldDef& def) override;

private:
	const AnimationRegistry* _animationRegistry{ nullptr };
	FrameworkRuntime* _framework{ nullptr };
	const WorldId* _bootstrapWorldId{ nullptr };
};

class ServerWorldBootstrapDefinitionProvider final
	: public IWorldDefinitionProvider {
public:
	bool RegisterExecutionSources(
		ExecutionSourceRegistry& sourceRegistry) const override;

	bool RegisterExecutionModels(
		const ExecutionSourceRegistry& sourceRegistry,
		WorldExecutionModelRegistry& executionModelRegistry) const override;

	bool RegisterTransferProfiles(
		WorldTransferProfileRegistry& transferProfileRegistry) const override;

	bool RegisterWorldDefs(
		WorldRegistry& worldRegistry) const override;
};
