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
		explicit ServerGameplayWorldImpl(
			const AnimationRegistry* animationRegistry)
			: _animationRegistry(animationRegistry)
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
			(void)runtime;
			// TODO: Dispatch initial SpawnSetDef once map/spawn content is defined.
			return true;
		}

		void OnStop(WorldRuntime& runtime) override
		{
			(void)runtime;
		}

	private:
		const AnimationRegistry* _animationRegistry{ nullptr };
	};

	class PlazaBootstrapWorldImpl final : public IWorldInstanceImpl {
	public:
		PlazaBootstrapWorldImpl(
			const AnimationRegistry* animationRegistry,
			FrameworkRuntime* framework,
			const WorldId* bootstrapWorldId)
			: _animationRegistry(animationRegistry)
			, _framework(framework)
			, _bootstrapWorldId(bootstrapWorldId)
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
				_bootstrapWorldId == nullptr ||
				!_bootstrapWorldId->IsValid())
			{
				return true;
			}

			SpawnAIEntity(
				*_framework,
				runtime,
				CharacterId::Imp,
				*_bootstrapWorldId,
				480.167800f,
				481.655600f);
			return true;
		}

		void OnStop(WorldRuntime& runtime) override
		{
			(void)runtime;
		}

	private:
		static void SpawnAIEntity(
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
			params.position = { spawnX, 5.508454f, spawnZ };
			params.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
			params.netId = netId;

			GetGlobalCharacterAspectRegistry().Assemble(
				runtime,
				aiEntity,
				*characterDef,
				params);
		}

		const AnimationRegistry* _animationRegistry{ nullptr };
		FrameworkRuntime* _framework{ nullptr };
		const WorldId* _bootstrapWorldId{ nullptr };
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
	const WorldDef& def)
{
	switch (def.id) {
	case WorldDefId::Plaza:
		return std::make_unique<PlazaBootstrapWorldImpl>(
			_animationRegistry,
			_framework,
			_bootstrapWorldId);
	case WorldDefId::Village:
	case WorldDefId::Castle:
	case WorldDefId::Final:
	case WorldDefId::Pvp:
		return std::make_unique<ServerGameplayWorldImpl>(
			_animationRegistry);
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
