#include "pch.h"
#include "ServerWorldBootstrap.h"

#include <array>
#include <span>
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
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"
#include "WorldTransferProfile.h"
#include "WorldTransferProfileRegistry.h"

namespace
{
	constexpr ExecToken kPlazaBootstrapExecToken = 1;
	constexpr WorldExecutionModelKey kPlazaBootstrapExecutionModelKey = 1;

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

	class PlazaBootstrapExecution final {
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

	struct DemoAISpawn
	{
		CharacterId characterId{ CharacterId::Imp };
		float x{ 0.0f };
		float z{ 0.0f };
	};

	std::span<const DemoAISpawn> ResolveDemoAISpawns(WorldDefId worldDefId)
	{
		static constexpr std::array<DemoAISpawn, 1> plazaSpawns
		{
			//DemoAISpawn{ CharacterId::Imp, 480.167800f, 481.655600f }
			//DemoAISpawn{ CharacterId::DemonStriker, 480.167800f, 481.655600f }
			DemoAISpawn{ CharacterId::DemonExecutioner, 500.167800f, 481.655600f }
		};
		static constexpr std::array<DemoAISpawn, 1> villageSpawns{
			DemoAISpawn{ CharacterId::DemonExecutioner, 500.167800f, 481.655600f },
			//DemoAISpawn{ CharacterId::Imp, 484.0f, 478.0f }
		};
		static constexpr std::array<DemoAISpawn, 3> castleSpawns{
			DemoAISpawn{ CharacterId::Imp, 476.0f, 484.0f },
			DemoAISpawn{ CharacterId::Imp, 482.0f, 480.0f },
			DemoAISpawn{ CharacterId::Imp, 488.0f, 476.0f }
		};
		static constexpr std::array<DemoAISpawn, 1> finalSpawns{
			DemoAISpawn{ CharacterId::Imp, 482.0f, 482.0f }
		};

		switch (worldDefId) {
		case WorldDefId::Plaza:
			return plazaSpawns;
		case WorldDefId::Village:
			return villageSpawns;
		case WorldDefId::Castle:
			return castleSpawns;
		case WorldDefId::Final:
			return finalSpawns;
		default:
			return {};
		}
	}

	void SpawnAIEntity(
		FrameworkRuntime& framework,
		WorldRuntime& runtime,
		CharacterId characterId,
		WorldId worldId,
		float spawnX,
		float spawnZ)
	{
		const CharacterDef* characterDef = FindCharacterDef(characterId);
		if (characterDef == nullptr ||
			!characterDef->ai.has_value() ||
			!AIFSMRegistry::IsArchetypeSupported(characterDef->ai->aiType))
		{
			return;
		}

		const Entity aiEntity = runtime.ReserveEntity();
		if (aiEntity.IsNull())
		{
			return;
		}

		const NetId netId = framework.BindEntityToNet(worldId, aiEntity);

		AssembleParams params{};
		params.position = { spawnX, 48.737797f, spawnZ };
		params.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		params.netId = netId;

		GetGlobalCharacterAspectRegistry().Assemble(
			runtime,
			aiEntity,
			*characterDef,
			params);
	}

	void SpawnDemoWorldEntities(
		FrameworkRuntime* framework,
		WorldRuntime& runtime,
		WorldId worldId,
		WorldDefId worldDefId)
	{
		if (framework == nullptr || !worldId.IsValid())
		{
			return;
		}

		for (const DemoAISpawn& spawn : ResolveDemoAISpawns(worldDefId))
		{
			SpawnAIEntity(
				*framework,
				runtime,
				spawn.characterId,
				worldId,
				spawn.x,
				spawn.z);
		}
	}

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
			WorldDefId worldDefId)
			: _animationRegistry(animationRegistry)
			, _framework(framework)
			, _worldId(worldId)
			, _worldDefId(worldDefId)
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
			SpawnDemoWorldEntities(
				_framework,
				runtime,
				_worldId,
				_worldDefId);
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
		WorldDefId _worldDefId{ WorldDefId::None };
	};

	class PlazaBootstrapWorldImpl final : public IWorldInstanceImpl {
	public:
		PlazaBootstrapWorldImpl(
			const AnimationRegistry* animationRegistry,
			FrameworkRuntime* framework,
			WorldId worldId)
			: _animationRegistry(animationRegistry)
			, _framework(framework)
			, _worldId(worldId)
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
			SpawnDemoWorldEntities(
				_framework,
				runtime,
				_worldId,
				WorldDefId::Plaza);
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
		return std::make_unique<PlazaBootstrapWorldImpl>(
			_animationRegistry,
			_framework,
			worldId);
	case WorldDefId::Village:
	case WorldDefId::Castle:
	case WorldDefId::Final:
	case WorldDefId::Pvp:
		return std::make_unique<ServerGameplayWorldImpl>(
			_animationRegistry,
			_framework,
			worldId,
			def.id);
	default:
		return nullptr;
	}
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionSources(
	ExecutionSourceRegistry& sourceRegistry) const
{
	ExecutionSourceDesc desc{};
	desc.token = kPlazaBootstrapExecToken;
	desc.phase = ExecPhase::Simulate;
	desc.lane = ExecLane::Main;
	desc.kind = ExecNodeKind::StaticSystem;
	desc.flags =
		static_cast<uint32_t>(ExecNodeFlag_NoThrow) |
		static_cast<uint32_t>(ExecNodeFlag_MainThreadOnly);
	desc.fn = &PlazaBootstrapExecution::ExecuteGraphSystems;
	desc.debugName = "PlazaBootstrap.GraphSystems";
	return sourceRegistry.Register(desc);
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionModels(
	const ExecutionSourceRegistry& sourceRegistry,
	WorldExecutionModelRegistry& executionModelRegistry) const
{
	WorldExecutionModel model{};
	model.key = kPlazaBootstrapExecutionModelKey;
	model.simulateSources.push_back(kPlazaBootstrapExecToken);
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
		kPlazaBootstrapExecutionModelKey;

	return
		worldRegistry.RegisterWorldDef(CreatePlazaWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreateVillageWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreateCastleWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreateFinalWorldDef(executionModelKey)) &&
		worldRegistry.RegisterWorldDef(CreatePvpWorldDef(executionModelKey));
}
