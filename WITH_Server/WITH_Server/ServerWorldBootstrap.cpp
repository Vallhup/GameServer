#include "pch.h"
#include "ServerWorldBootstrap.h"

#include <string>

#include "AIFSMRegistry.h"
#include "Aspect/CharacterAspectRegistry.h"
#include "Aspect/ICharacterAspect.h"
#include "CharacterDef.h"
#include "ECS/System/GameplaySystemRegistration.h"
#include "ExecutionContextTypes.h"
#include "ExecutionSourceTypes.h"
#include "FrameworkRuntime.h"
#include "PlayerCharacterTransferSerializer.h"
#include "SpawnSetDef.h"
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"
#include "WorldTransferProfile.h"
#include "WorldTransferProfileRegistry.h"

namespace
{
	constexpr ExecToken kGameplayBootstrapExecToken = 1;
	constexpr WorldExecutionModelKey kGameplayBootstrapExecutionModelKey = 1;

	class ExecScopeNetBindingResolver final : public IWorldNetBindingResolver {
	public:
		explicit ExecScopeNetBindingResolver(
			const NodeExecContext& context) noexcept
			: _context(context)
		{
		}

		bool TryResolveEntity(
			const NetId& netId,
			Entity& outEntity) const noexcept override
		{
			outEntity = Entity::Null();

			ExecutionOps* const ops = _context.TryGetOps();
			const WorldId worldId = _context.TryGetWorldId();
			if (ops == nullptr || !worldId.IsValid())
			{
				return false;
			}

			return ops->TryResolveEntity(worldId, netId, outEntity);
		}

	private:
		const NodeExecContext& _context;
	};

	class GameplayBootstrapExecution final {
	public:
		static ExecCallResult ExecuteGraphSystems(NodeExecContext& context)
		{
			WorldRuntime* const runtime = context.TryGetRuntime();
			if (runtime == nullptr)
			{
				return ExecCallResult::Failed;
			}

			const ExecScopeNetBindingResolver netBindingResolver(context);
			const WorldSystemServices services{
				.netBindingResolver = &netBindingResolver
			};

			return runtime->ExecuteSystems(SystemPhase::Graph, services)
				? ExecCallResult::Success
			: ExecCallResult::Failed;
		}
	};

	class ServerGameplayRuntimeBootstrap final {
	public:
		static bool RegisterRuntime(
			WorldRuntime& runtime,
			const AnimationRegistry* animationRegistry)
		{
			const CharacterAspectRegistry& aspects =
				GetGlobalCharacterAspectRegistry();

			aspects.RegisterStoragesAll(runtime);
			RegisterGameplayRuntimeSystems(runtime, animationRegistry);
			ValidateCharacterDefs(aspects);
			return true;
		}

	private:
		static void ValidateCharacterDefs(
			const CharacterAspectRegistry& aspects)
		{
			for (const CharacterDef& def : GetCharacterDefs())
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
			WorldId worldId,
			const WorldDef& worldDef)
			: _animationRegistry(animationRegistry)
			, _framework(framework)
			, _worldId(worldId)
			, _worldDef(&worldDef)
		{
		}

		bool OnCreate(WorldRuntime& runtime) override
		{
			return ServerGameplayRuntimeBootstrap::RegisterRuntime(
				runtime,
				_animationRegistry);
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
				FindSpawnSetDef(_worldDef->spawn.initialSpawnSetId);
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
					FindCharacterDef(entry.characterId);
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

std::unique_ptr<IWorldInstanceImpl> ServerWorldBootstrapFactory::Create(
	const WorldDef& def,
	WorldId worldId)
{
	switch (def.id) {
	case WorldDefId::Plaza:
	case WorldDefId::Village:
	case WorldDefId::Castle:
	case WorldDefId::Final:
	case WorldDefId::Pvp:
		return std::make_unique<ServerGameplayWorldImpl>(
			_animationRegistry,
			_framework,
			worldId,
			def);
	default:
		return nullptr;
	}
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionSources(
	ExecutionSourceRegistry& sourceRegistry) const
{
	ExecutionSourceDesc desc{};
	desc.token = kGameplayBootstrapExecToken;
	desc.phase = ExecPhase::Simulate;
	desc.lane = ExecLane::Main;
	desc.kind = ExecNodeKind::StaticSystem;
	desc.flags =
		static_cast<uint32_t>(ExecNodeFlag_NoThrow) |
		static_cast<uint32_t>(ExecNodeFlag_MainThreadOnly);
	desc.fn = &GameplayBootstrapExecution::ExecuteGraphSystems;
	desc.debugName = "GameplayBootstrap.GraphSystems";
	return sourceRegistry.Register(desc);
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionModels(
	const ExecutionSourceRegistry& sourceRegistry,
	WorldExecutionModelRegistry& executionModelRegistry) const
{
	WorldExecutionModel model{};
	model.key = kGameplayBootstrapExecutionModelKey;
	model.simulateSources.push_back(kGameplayBootstrapExecToken);
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
