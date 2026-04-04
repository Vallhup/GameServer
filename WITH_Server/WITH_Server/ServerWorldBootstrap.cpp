#include "pch.h"
#include "ServerWorldBootstrap.h"

#include "ExecutionContextTypes.h"
#include "ExecutionSourceTypes.h"
#include "ECS/System/GameplaySystemRegistration.h"
#include "RepComponent.h"
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"

namespace
{
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
			(void)runtime;
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
