#include "pch.h"

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "CheatCommandPolicy.h"
#include "DynamicTaskTypes.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/Phase2/ResolveAbilityStateSystem.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionSourceTypes.h"
#include "NetworkRuntime.h"
#include "PacketHandlers.h"
#include "SystemMetaHelper.h"

struct ExecutionGraphBuilderTestHook
{
	static void AppendDynamicNodes(
		const ExecutionGraphBuilder& builder,
		const DynamicTaskFrozenBatch& batch,
		const DynamicTaskTypeRegistry& typeRegistry,
		const ExecutionSourceRegistry& sourceRegistry,
		FrameTaskGraph& graph,
		std::vector<std::vector<ExecNodeId>>& predLists,
		std::vector<std::vector<ExecNodeId>>& succLists,
		DynamicTaskFrameTable& frameTable,
		BuildResult& result)
	{
		builder.AppendDynamicNodes(
			batch,
			typeRegistry,
			sourceRegistry,
			nullptr,
			graph,
			predLists,
			succLists,
			frameTable,
			result);
	}

	static void FinalizeEdgePool(
		const ExecutionGraphBuilder& builder,
		FrameTaskGraph& graph,
		const std::vector<std::vector<ExecNodeId>>& predLists,
		const std::vector<std::vector<ExecNodeId>>& succLists,
		const ExecutionGraphBuildPolicy& policy)
	{
		builder.FinalizeEdgePool(
			graph,
			predLists,
			succLists,
			policy);
	}

	static bool ValidateGraph(
		const ExecutionGraphBuilder& builder,
		const FrameTaskGraph& graph,
		const ExecutionGraphBuildPolicy& policy,
		BuildResult& result)
	{
		return builder.ValidateGraph(graph, policy, result);
	}
};

namespace
{
	ExecCallResult NoopStaticSystem(NodeExecContext&)
	{
		return ExecCallResult::Success;
	}

	const DynamicTaskTypeDesc* FindTaskType(
		const DynamicTaskTypeRegistry& registry,
		std::string_view debugName)
	{
		for (DynamicTaskTypeId typeId = 1;
			typeId <= registry.TypeCount();
			++typeId)
		{
			const DynamicTaskTypeDesc* const desc = registry.TryGet(typeId);
			if (desc != nullptr && desc->debugName == debugName)
			{
				return desc;
			}
		}
		return nullptr;
	}

	void Require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
	}

	void VerifyMoveCheatMoveGraphIsAcyclic()
	{
		ExecutionSourceRegistry sourceRegistry{};
		DynamicTaskTypeRegistry taskRegistry{};
		NetworkRuntime network{};
		DynamicTaskTypeId disconnectedTypeId{ InvalidDynamicTaskTypeId };
		RegisterServerPacketHandlers(
			taskRegistry,
			sourceRegistry,
			network,
			disconnectedTypeId);

		const DynamicTaskTypeDesc* const moveDesc =
			FindTaskType(taskRegistry, "Pkt_CS_MOVE");
		const DynamicTaskTypeDesc* const cheatDesc =
			FindTaskType(taskRegistry, "Pkt_CS_CHEAT_COMMAND");
		Require(moveDesc != nullptr, "move task descriptor missing");
		Require(cheatDesc != nullptr, "cheat task descriptor missing");

		static const std::array<AccessSpec, 1> kStaticAccesses{
			ReadImmediate(ComponentRes<ActorInputComp>())
		};

		ExecutionSourceDesc staticDesc{};
		staticDesc.token = sourceRegistry.AllocateToken();
		staticDesc.phase = ExecPhase::Simulate;
		staticDesc.lane = ExecLane::Parallel;
		staticDesc.kind = ExecNodeKind::StaticSystem;
		staticDesc.tag = Tag<ResolveAbilityStateSystem>();
		staticDesc.fn = &NoopStaticSystem;
		staticDesc.debugName = "ResolveAbilityStateSystemProbe";
		staticDesc.accesses = kStaticAccesses;
		Require(sourceRegistry.Register(staticDesc),
			"static probe source registration failed");

		FrameTaskGraph graph{};
		graph.scopeCount = 1;
		graph.scopeToWorld = { 1 };
		graph.simulateNodeCount = 1;

		ExecNodeRecord staticNode{};
		staticNode.id = 0;
		staticNode.scopeId = 0;
		staticNode.phase = ExecPhase::Simulate;
		staticNode.lane = ExecLane::Parallel;
		staticNode.kind = ExecNodeKind::StaticSystem;
		staticNode.sourceToken = staticDesc.token;
		graph.nodes.push_back(staticNode);

		std::vector<std::vector<ExecNodeId>> predLists(1);
		std::vector<std::vector<ExecNodeId>> succLists(1);

		auto makeRequest = [](DynamicTaskTypeId typeId, uint64_t sequence)
		{
			DynamicTaskRequest request{};
			request.typeId = typeId;
			request.scopeId = 0;
			request.requestFrameIndex = 1;
			request.sessionId = 1;
			request.submissionSequence = sequence;
			return request;
		};

		DynamicTaskFrozenBatch batch{};
		batch.requests = {
			makeRequest(moveDesc->typeId, 1),
			makeRequest(cheatDesc->typeId, 2),
			makeRequest(moveDesc->typeId, 3)
		};
		batch.Sort();

		DynamicTaskFrameTable frameTable{};
		BuildResult result{};
		ExecutionGraphBuilder builder{};
		ExecutionGraphBuildPolicy policy{};

		ExecutionGraphBuilderTestHook::AppendDynamicNodes(
			builder,
			batch,
			taskRegistry,
			sourceRegistry,
			graph,
			predLists,
			succLists,
			frameTable,
			result);
		ExecutionGraphBuilderTestHook::FinalizeEdgePool(
			builder,
			graph,
			predLists,
			succLists,
			policy);

		Require(
			ExecutionGraphBuilderTestHook::ValidateGraph(
				builder,
				graph,
				policy,
				result),
			"MOVE-CHEAT-MOVE created a cyclic execution graph");
	}
}

void RunCheatCommandSmokeTests()
{
	assert(CheatCommandPolicy::ResolveTeleportBoss(WorldDefId::Village) ==
		CharacterId::BigDemonWarrior);
	assert(CheatCommandPolicy::ResolveTeleportBoss(WorldDefId::Castle) ==
		CharacterId::Tank);
	assert(CheatCommandPolicy::ResolveTeleportBoss(WorldDefId::Final) ==
		CharacterId::None);

	assert(CheatCommandPolicy::ResolveKillBoss(WorldDefId::Village) ==
		CharacterId::BigDemonWarrior);
	assert(CheatCommandPolicy::ResolveKillBoss(WorldDefId::Castle) ==
		CharacterId::Tank);
	assert(CheatCommandPolicy::ResolveKillBoss(WorldDefId::Final) ==
		CharacterId::FinalBoss);
	assert(CheatCommandPolicy::ResolveKillBoss(WorldDefId::Plaza) ==
		CharacterId::None);

	assert(CheatCommandPolicy::CanTransferToFinal(WorldDefId::Plaza));
	assert(CheatCommandPolicy::CanTransferToFinal(WorldDefId::Village));
	assert(CheatCommandPolicy::CanTransferToFinal(WorldDefId::Castle));
	assert(CheatCommandPolicy::CanTransferToFinal(WorldDefId::Pvp));
	assert(!CheatCommandPolicy::CanTransferToFinal(WorldDefId::Final));
	assert(!CheatCommandPolicy::CanTransferToFinal(WorldDefId::None));

	VerifyMoveCheatMoveGraphIsAcyclic();

	std::cout << "[PASS] CheatCommand smoke\n";
}
