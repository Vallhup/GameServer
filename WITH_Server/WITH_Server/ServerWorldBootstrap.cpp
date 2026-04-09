#include "pch.h"
#include "ServerWorldBootstrap.h"

#include <string>

#include "Aspect/CharacterAspectRegistry.h"
#include "Aspect/ICharacterAspect.h"
#include "CharacterIdPolicy.h"
#include "ECS/System/GameplaySystemRegistration.h"
#include "ExecutionContextTypes.h"
#include "ExecutionSourceTypes.h"
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"
#include "CharacterDef.h"
#include "FrameworkRuntime.h"
#include "ServerApp.h"
#include "AIFSMRegistry.h"

namespace
{
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

	void SpawnAIEntity(
		FrameworkRuntime& framework,
		WorldRuntime& runtime,
		CharacterId characterId,
		WorldId worldId,
		float spawnX,
		float spawnZ)
	{
		const CharacterDef* characterDef = FindCharacterDef(characterId);
		if (characterDef == nullptr)
		{
			return;
		}

		// AI 정의가 없거나, 정의된 archetype 의 FSM bundle 이 아직 구현되지 않았다면
		// 스폰을 거부한다. 정의는 있지만 동작하지 않는 "frozen AI" 를 막기 위함.
		// (동일 체크가 AIControlAspect::Validate 에서 부팅 시에도 수행되지만,
		//  런타임 스폰 경로에서도 방어적으로 한 번 더 확인한다.)
		if (!characterDef->ai.has_value() ||
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
		params.position = { spawnX, 50.0f, spawnZ };
		params.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		params.netId = netId;

		GetGlobalCharacterAspectRegistry().Assemble(
			runtime, aiEntity, *characterDef, params);
	}

	constexpr ExecToken kSquareBootstrapExecToken = 1;
	constexpr WorldExecutionModelKey kSquareBootstrapExecutionModelKey = 1;

	ExecCallResult ExecuteSquareBootstrapGraphSystems(NodeExecContext& context)
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

	class SquareBootstrapWorldImpl final : public IWorldInstanceImpl {
	public:
		explicit SquareBootstrapWorldImpl(
			const AnimationRegistry* animationRegistry,
			FrameworkRuntime* framework,
			const WorldId* bootstrapWorldId)
			: _animationRegistry(animationRegistry)
			, _framework(framework)
			, _bootstrapWorldId(bootstrapWorldId)
		{
		}

	public:
		bool OnCreate(WorldRuntime& runtime) override
		{
			const CharacterAspectRegistry& aspects =
				GetGlobalCharacterAspectRegistry();

			aspects.RegisterStoragesAll(runtime);
			RegisterGameplayRuntimeSystems(runtime, _animationRegistry);

			// 부팅 검증: 모든 캐릭터 Def 가 필요한 전제 조건을 만족하는지 확인한다.
			// 현재는 실패 시 boot 를 중단하지 않는다 (예: FinalBoss 의 FSM bundle 미지원은
			// 의도된 상태이며 OnStart 가 해당 캐릭터 스폰을 우회한다).
			// TODO: 로깅 인프라가 준비되면 실패 내역을 기록한다.
			for (const CharacterDef& def : GetCharacterDefs())
			{
				std::string validationError;
				(void)aspects.ValidateAll(def, validationError);
			}
			return true;
		}

		bool OnStart(WorldRuntime& runtime) override
		{
			if (_framework == nullptr ||
				_bootstrapWorldId == nullptr ||
				!_bootstrapWorldId->IsValid())
			{
				return true;
			}

<<<<<<< HEAD
			// FinalBoss 는 FinalBossMonster FSM bundle 이 아직 없어 스폰이 거부된다.
			// 임시로 NormalMonster archetype 인 Imp 를 스폰한다.
			SpawnAIEntity(*_framework, runtime, CharacterId::Imp,
				*_bootstrapWorldId, 156.0f, 650.0f);
=======
			SpawnAIEntity(*_framework, runtime, CharacterId::FinalBoss,
				*_bootstrapWorldId, 508.167800f, 481.655600f);
>>>>>>> 0f5d17ee9a637e1f24802479d22b3439e2fb5e80
			return true;
		}

		void OnStop(WorldRuntime& runtime) override
		{
			(void)runtime;
		}

	private:
		const AnimationRegistry* _animationRegistry{ nullptr };
		FrameworkRuntime* _framework{ nullptr };
		const WorldId* _bootstrapWorldId{ nullptr };
	};

	WorldDef MakeSquareWorldDef()
	{
		WorldDef def{};
		def.id = WorldDefId::Square;
		def.name = "Square_0";

		def.topology.kind = WorldKind::Hub;
		def.topology.instanceType = WorldInstanceType::Persistent;

		def.entryPolicy.creationPolicy = CreationPolicy::PreCreated;
		def.entryPolicy.joinPolicy = JoinPolicy::FreeJoin;
		def.entryPolicy.maxPlayerCount = 5000;
		def.entryPolicy.allowReEntry = true;
		def.entryPolicy.destroyWhenEmpty = false;
		def.entryPolicy.emptyDestroyDelaySec = std::nullopt;
		def.entryPolicy.fallbackWorldDefId = std::nullopt;

		// Map/spawn resources are not specified yet, so keep the typed fields at zero.
		def.map.resourceId = 0;
		def.map.defaultPlayerSpawnPointId = 0;
		def.map.namedSpawnPoints.clear();
		def.map.navigationProfileId = std::nullopt;
		def.map.environmentTags.clear();

		def.spawn.initialSpawnSetId = SpawnSetId::None;
		def.spawn.respawnSpawnSetId = std::nullopt;

		def.progressRule.clearType = WorldClearConditionType::None;
		def.progressRule.failType = WorldFailConditionType::None;
		def.progressRule.completionType = WorldCompletionActionType::None;
		def.progressRule.completionDelaySec = std::nullopt;
		def.progressRule.autoCloseOnComplete = false;

		def.linkRules.clear();
		def.executionModelKey = kSquareBootstrapExecutionModelKey;
		def.transferProfileId = InvalidWorldTransferProfileId;
		return def;
	}
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
	case WorldDefId::Square:
		return std::make_unique<SquareBootstrapWorldImpl>(
			_animationRegistry,
			_framework,
			_bootstrapWorldId);
	default:
		return nullptr;
	}
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionSources(
	ExecutionSourceRegistry& sourceRegistry) const
{
	ExecutionSourceDesc desc{};
	desc.token = kSquareBootstrapExecToken;
	desc.phase = ExecPhase::Simulate;
	desc.lane = ExecLane::Main;
	desc.kind = ExecNodeKind::StaticSystem;
	desc.flags =
		static_cast<uint32_t>(ExecNodeFlag_NoThrow) |
		static_cast<uint32_t>(ExecNodeFlag_MainThreadOnly);
	desc.fn = &ExecuteSquareBootstrapGraphSystems;
	desc.debugName = "SquareBootstrap.GraphSystems";
	return sourceRegistry.Register(desc);
}

bool ServerWorldBootstrapDefinitionProvider::RegisterExecutionModels(
	const ExecutionSourceRegistry& sourceRegistry,
	WorldExecutionModelRegistry& executionModelRegistry) const
{
	WorldExecutionModel model{};
	model.key = kSquareBootstrapExecutionModelKey;
	model.simulateSources.push_back(kSquareBootstrapExecToken);
	return executionModelRegistry.Register(model, sourceRegistry);
}

bool ServerWorldBootstrapDefinitionProvider::RegisterWorldDefs(
	WorldRegistry& worldRegistry) const
{
	return worldRegistry.RegisterWorldDef(MakeSquareWorldDef());
}
