#include "pch.h"
#include "ServerWorldBootstrap.h"

#include <string>

#include "AIFSMRegistry.h"
#include "Aspect/CharacterAspectRegistry.h"
#include "Aspect/ICharacterAspect.h"
#include "AutoSystemBridge.h"
#include "CharacterDef.h"
#include "ECS/System/GameplaySystemRegistration.h"
#include "ExecutionSourceTypes.h"
#include "FrameworkRuntime.h"
#include "GameDataCatalog.h"
#include "PlayerCharacterTransferSerializer.h"
#include "SpawnSetDef.h"
#include "SystemManager.h"
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"
#include "WorldTransferProfile.h"
#include "WorldTransferProfileRegistry.h"

namespace
{
	constexpr WorldExecutionModelKey kGameplayBootstrapExecutionModelKey = 1;

	class ServerGameplayRuntimeBootstrap final {
	public:
		static bool RegisterRuntime(
			WorldRuntime& runtime,
			const AnimationRegistry* animationRegistry,
			const GameDataCatalog& catalog)
		{
			const CharacterAspectRegistry& aspects =
				GetGlobalCharacterAspectRegistry();

			aspects.RegisterStoragesAll(runtime);
			GameplaySystemRegistrar registrar(animationRegistry);
			registrar.Register(runtime);
			ValidateCharacterDefs(aspects, catalog);
			return true;
		}

	private:
		static void ValidateCharacterDefs(
			const CharacterAspectRegistry& aspects,
			const GameDataCatalog& catalog)
		{
			for (const CharacterDef& def : catalog.Characters().GetAll())
			{
				std::string validationError;
				(void)aspects.ValidateAll(def, validationError);
			}
		}
	};

	class ServerGameplayWorldImpl final : public IWorldInstanceImpl {
	public:
		ServerGameplayWorldImpl(
			const AnimationRegistry* animationRegistry,
			FrameworkRuntime* framework,
			const GameDataCatalog& catalog,
			WorldId worldId,
			const WorldDef& worldDef)
			: _animationRegistry(animationRegistry)
			, _framework(framework)
			, _catalog(&catalog)
			, _worldId(worldId)
			, _worldDef(&worldDef)
		{
		}

		bool OnCreate(WorldRuntime& runtime) override
		{
			if (!ServerGameplayRuntimeBootstrap::RegisterRuntime(
				runtime,
				_animationRegistry,
				*_catalog))
			{
				return false;
			}

			if (_framework == nullptr)
			{
				return false;
			}

			return _framework->BindRuntimeSystems(runtime, ExecPhase::Simulate);
		}

		bool OnStart(WorldRuntime& runtime) override
		{
			if (_framework == nullptr ||
				_worldDef == nullptr ||
				!_worldId.IsValid())
			{
				return true;
			}

			const SpawnSetDef* const spawnSet =
				_catalog->SpawnSets().Find(_worldDef->spawn.initialSpawnSetId);
			if (spawnSet == nullptr)
			{
				return true;
			}

			for (const SpawnEntryDef& entry : spawnSet->entries)
			{
				if (entry.type != SpawnConditionType::Always &&
					entry.type != SpawnConditionType::OnWorldStart)
				{
					continue;
				}

				const SpawnPointDef* spawnPoint = nullptr;
				for (const SpawnPointDef& candidate : _worldDef->map.spawnPoints)
				{
					if (candidate.id == entry.spawnPointId)
					{
						spawnPoint = &candidate;
						break;
					}
				}

				if (spawnPoint == nullptr)
				{
					continue;
				}

				const CharacterDef* characterDef =
					_catalog->Characters().Find(entry.characterId);
				if (characterDef == nullptr ||
					!characterDef->ai.has_value() ||
					!AIFSMRegistry::IsArchetypeSupported(characterDef->ai->aiType))
				{
					continue;
				}

				for (uint8_t i = 0; i < entry.count; ++i)
				{
					const Entity aiEntity = runtime.ReserveEntity();
					if (aiEntity.IsNull())
					{
						continue;
					}

					const NetId netId =
						_framework->BindEntityToNet(_worldId, aiEntity);
					if (!netId.IsValid())
					{
						runtime.DeferredDestroyEntity(aiEntity);
						continue;
					}

					AssembleParams params{};
					params.position = {
						spawnPoint->position.x,
						spawnPoint->position.y,
						spawnPoint->position.z
					};
					params.rotation = {
						spawnPoint->rotation.x,
						spawnPoint->rotation.y,
						spawnPoint->rotation.z,
						spawnPoint->rotation.w
					};
					params.netId = netId;

					GetGlobalCharacterAspectRegistry().Assemble(
						runtime,
						aiEntity,
						*characterDef,
						params);
				}
			}

			return true;
		}

		void OnStop(WorldRuntime& runtime) override
		{
			(void)runtime;
		}

	private:
		const AnimationRegistry* _animationRegistry{ nullptr };
		FrameworkRuntime* _framework{ nullptr };
		const GameDataCatalog* _catalog{ nullptr };
		WorldId _worldId{ WorldId::Invalid() };
		const WorldDef* _worldDef{ nullptr };
	};
}

void ServerWorldBootstrapFactory::SetAnimationRegistry(
	const AnimationRegistry* animationRegistry) noexcept
{
	_animationRegistry = animationRegistry;
}

void ServerWorldBootstrapFactory::SetFramework(
	FrameworkRuntime* framework) noexcept
{
	_framework = framework;
}

void ServerWorldBootstrapFactory::SetBootstrapWorldId(
	const WorldId* worldId) noexcept
{
	_bootstrapWorldId = worldId;
}

void ServerWorldBootstrapFactory::SetGameDataCatalog(
	const GameDataCatalog* catalog) noexcept
{
	_gameDataCatalog = catalog;
}

std::unique_ptr<IWorldInstanceImpl> ServerWorldBootstrapFactory::Create(
	const WorldDef& def,
	WorldId worldId)
{
	if (_gameDataCatalog == nullptr)
		return nullptr;

	switch (def.id) {
	case WorldDefId::Plaza:
	case WorldDefId::Village:
	case WorldDefId::Castle:
	case WorldDefId::Final:
	case WorldDefId::Pvp:
		return std::make_unique<ServerGameplayWorldImpl>(
			_animationRegistry,
			_framework,
			*_gameDataCatalog,
			worldId,
			def);
	default:
		return nullptr;
	}
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionSources(
	ExecutionSourceRegistry& sourceRegistry) const
{
	SystemManager systemManager;
	GameplaySystemRegistrar registrar(nullptr);
	registrar.Register(systemManager);

	AutoSystemBridge bridge;
	const AutoSystemBridge::BridgeResult result =
		bridge.RegisterSources(
			ExecPhase::Simulate,
			systemManager,
			sourceRegistry);

	return result.success;
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionModels(
	const ExecutionSourceRegistry& sourceRegistry,
	WorldExecutionModelRegistry& executionModelRegistry) const
{
	SystemManager systemManager;
	GameplaySystemRegistrar registrar(nullptr);
	registrar.Register(systemManager);

	WorldExecutionModel model{};
	model.key = kGameplayBootstrapExecutionModelKey;

	AutoSystemBridge bridge;
	const AutoSystemBridge::BridgeResult result =
		bridge.BuildModelFromRegisteredSources(
			ExecPhase::Simulate,
			systemManager,
			sourceRegistry,
			model);
	if (!result.success)
	{
		return false;
	}

	return executionModelRegistry.Register(model, sourceRegistry);
}

bool ServerWorldBootstrapDefinitionProvider::RegisterTransferProfiles(
	WorldTransferProfileRegistry& transferProfileRegistry) const
{
	auto profile = std::make_unique<WorldTransferProfile>();
	auto serializer = CreatePlayerCharacterTransferSerializer();
	if (!serializer)
	{
		return false;
	}

	const WorldTransferSerializerId serializerId =
		serializer->GetSerializerId();
	if (!profile->Add(serializerId, std::move(serializer)))
	{
		return false;
	}

	return transferProfileRegistry.Register(
		PlayerCharacterWorldTransferProfileId,
		std::move(profile));
}

bool ServerWorldBootstrapDefinitionProvider::RegisterWorldDefs(
	WorldRegistry& worldRegistry) const
{
	const WorldExecutionModelKey executionModelKey =
		kGameplayBootstrapExecutionModelKey;

	return
		worldRegistry.RegisterWorldDef(CreatePlazaWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreateVillageWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreateCastleWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreateFinalWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreatePvpWorldDef(executionModelKey));
}
