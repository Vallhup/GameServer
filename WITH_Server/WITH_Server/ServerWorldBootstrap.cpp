#include "pch.h"
#include "ServerWorldBootstrap.h"

#include "ExecutionContextTypes.h"
#include "ExecutionSourceTypes.h"
#include "RepComponent.h"
#include "WorldContentIds.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRegistry.h"

namespace
{
	constexpr ExecToken kSquareBootstrapExecToken = 1;
	constexpr WorldExecutionModelKey kSquareBootstrapExecutionModelKey = 1;

	ExecCallResult ExecuteSquareBootstrapNoOp(NodeExecContext& context)
	{
		return context.TryGetRuntime() != nullptr
			? ExecCallResult::Success
			: ExecCallResult::Failed;
	}

	class SquareBootstrapWorldImpl final : public IWorldInstanceImpl {
	public:
		bool OnCreate(WorldRuntime& runtime) override
		{
			runtime.RegisterStorage<ReplicatedTag>();
			runtime.RegisterStorage<SpawnTypeComp>();
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

std::unique_ptr<IWorldInstanceImpl> ServerWorldBootstrapFactory::Create(
	const WorldDef& def)
{
	switch (def.id) {
	case WorldDefId::Square:
		return std::make_unique<SquareBootstrapWorldImpl>();
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
	desc.fn = &ExecuteSquareBootstrapNoOp;
	desc.debugName = "SquareBootstrap.NoOpSimulate";
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
