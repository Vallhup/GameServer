#pragma once

#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"
#include "WorldId.h"

class AnimationRegistry;
class FrameworkRuntime;
class GameDataCatalog;
class ServerApp;

class ServerWorldBootstrapFactory final : public IWorldInstanceFactory {
public:
	void SetAnimationRegistry(
		const AnimationRegistry* animationRegistry) noexcept;
	void SetFramework(FrameworkRuntime* framework) noexcept;
	void SetBootstrapWorldId(const WorldId* worldId) noexcept;
	void SetGameDataCatalog(const GameDataCatalog* catalog) noexcept;

	std::unique_ptr<IWorldInstanceImpl> Create(
		const WorldDef& def,
		WorldId worldId) override;

private:
	const AnimationRegistry* _animationRegistry{ nullptr };
	FrameworkRuntime* _framework{ nullptr };
	const WorldId* _bootstrapWorldId{ nullptr };
	const GameDataCatalog* _gameDataCatalog{ nullptr };
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
