#include "pch.h"
#include "ServerWorldBootstrap.h"

#include "CharacterIdPolicy.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/GameplaySystemRegistration.h"
#include "ExecutionContextTypes.h"
#include "ExecutionSourceTypes.h"
#include "RepComponent.h"
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"

namespace
{
	void SpawnAIEntity(
		WorldRuntime& runtime,
		CharacterId characterId,
		float spawnX,
		float spawnZ)
	{
		const CharacterDef* characterDef = FindCharacterDef(characterId);
		if (characterDef == nullptr)
		{
			return;
		}

		const Entity aiEntity = runtime.ReserveEntity();
		if (aiEntity.IsNull())
		{
			return;
		}

		// 공통 컴포넌트 (플레이어와 동일)
		runtime.DeferredAddComponent<ReplicatedTag>(aiEntity);
		runtime.DeferredUpsertComponent<SpawnTypeComp>(
			aiEntity,
			SpawnTypeComp{ .characterId = characterId });
		runtime.DeferredAddComponent<ActorInputComp>(aiEntity);
		runtime.DeferredAddComponent<ActionStateComp>(aiEntity);
		runtime.DeferredAddComponent<LocomotionStateComp>(aiEntity);
		runtime.DeferredAddComponent<ActionTimelineAdvanceComp>(aiEntity);
		runtime.DeferredAddComponent<AnimationPlaybackStateComp>(aiEntity);
		runtime.DeferredAddComponent<SampledAnimationPoseComp>(aiEntity);
		runtime.DeferredAddComponent<SkeletalCombatColliderComp>(aiEntity);
		runtime.DeferredUpsertComponent<WorldTransformComp>(
			aiEntity,
			WorldTransformComp{ .position = { spawnX, 0.0f, spawnZ }, .yawRad = 0.0f });
		runtime.DeferredAddComponent<LocomotionMoveDeltaComp>(aiEntity);
		runtime.DeferredAddComponent<ActionMoveDeltaComp>(aiEntity);
		runtime.DeferredAddComponent<ActionMoveRuntimeComp>(aiEntity);
		runtime.DeferredAddComponent<PreCollisionTransformComp>(aiEntity);
		runtime.DeferredAddComponent<BodyCollisionShapeComp>(aiEntity);
		runtime.DeferredAddComponent<NavMeshAgentStateComp>(aiEntity);
		runtime.DeferredAddComponent<BodyCollisionResolveComp>(aiEntity);
		runtime.DeferredAddComponent<PortalTriggerStateComp>(aiEntity);
		runtime.DeferredAddComponent<CombatColliderActivationComp>(aiEntity);
		runtime.DeferredAddComponent<CombatHitDedupStateComp>(aiEntity);
		runtime.DeferredAddComponent<PendingCombatResultComp>(aiEntity);

		CombatStatStateComp stats{};
		stats.currentHp      = static_cast<int32_t>(characterDef->stat.maxHp);
		stats.maxHp          = static_cast<int32_t>(characterDef->stat.maxHp);
		stats.currentStamina = static_cast<int32_t>(characterDef->stat.maxStamina);
		stats.maxStamina     = static_cast<int32_t>(characterDef->stat.maxStamina);
		stats.currentPoise   = static_cast<int32_t>(characterDef->stat.maxPoise);
		stats.maxPoise       = static_cast<int32_t>(characterDef->stat.maxPoise);
		stats.attackPower    = static_cast<int32_t>(characterDef->stat.attackPower);
		stats.defense        = static_cast<int32_t>(characterDef->stat.defense);
		stats.attackSpeed    = characterDef->stat.attackSpeed;
		stats.moveSpeed      = characterDef->stat.moveSpeed;
		runtime.DeferredUpsertComponent<CombatStatStateComp>(aiEntity, stats);

		runtime.DeferredAddComponent<BuffRuntimeStateComp>(aiEntity);
		runtime.DeferredAddComponent<PendingProjectileSpawnComp>(aiEntity);
		runtime.DeferredAddComponent<PendingActionPresentationEventComp>(aiEntity);
		runtime.DeferredAddComponent<DirtyFlagsComp>(aiEntity);
		runtime.DeferredAddComponent<ReplicationStatsComp>(aiEntity);

		// AI 전용 컴포넌트 (PlayerControlIdentityComp 없음)
		runtime.DeferredAddComponent<AIControlledTag>(aiEntity);
		runtime.DeferredAddComponent<AIPerceptionComp>(aiEntity);
		runtime.DeferredAddComponent<AIPerceptionTuningComp>(aiEntity);
		runtime.DeferredAddComponent<AIBlackboardComp>(aiEntity);
		runtime.DeferredAddComponent<AIDecisionComp>(aiEntity);
		runtime.DeferredAddComponent<AIDecisionTuningComp>(aiEntity);
		runtime.DeferredAddComponent<AIReactionComp>(aiEntity);
		runtime.DeferredAddComponent<AICommandFrameComp>(aiEntity);
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

		return runtime->ExecuteSystems(SystemPhase::Graph)
			? ExecCallResult::Success
			: ExecCallResult::Failed;
	}

	class SquareBootstrapWorldImpl final : public IWorldInstanceImpl {
	public:
		explicit SquareBootstrapWorldImpl(
			const AnimationRegistry* animationRegistry)
			: _animationRegistry(animationRegistry)
		{
		}

	public:
		bool OnCreate(WorldRuntime& runtime) override
		{
			runtime.RegisterStorage<ReplicatedTag>();
			runtime.RegisterStorage<SpawnTypeComp>();
			RegisterGameplayRuntimeStorages(runtime);
			RegisterGameplayRuntimeSystems(runtime, _animationRegistry);
			return true;
		}

		bool OnStart(WorldRuntime& runtime) override
		{
			SpawnAIEntity(runtime, CharacterId::Imp,  3.0f, 5.0f);
			SpawnAIEntity(runtime, CharacterId::Imp, -3.0f, 5.0f);
			return true;
		}

		void OnStop(WorldRuntime& runtime) override
		{
			(void)runtime;
		}

	private:
		const AnimationRegistry* _animationRegistry{ nullptr };
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

std::unique_ptr<IWorldInstanceImpl> ServerWorldBootstrapFactory::Create(
	const WorldDef& def)
{
	switch (def.id) {
	case WorldDefId::Square:
		return std::make_unique<SquareBootstrapWorldImpl>(_animationRegistry);
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
