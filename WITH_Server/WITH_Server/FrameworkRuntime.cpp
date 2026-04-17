#include "pch.h"
#include "FrameworkRuntime.h"

#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionOps.h"
#include "ExecutionSourceTypes.h"
#include "FrameworkFrameEventHarvester.h"
#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"
#include "NetIdRegistry.h"
#include "PresenceManager.h"
#include "TaskExecutor.h"
#include "WorldAdmissionService.h"
#include "WorldDefinitionBootstrap.h"
#include "WorldExecutionModelTypes.h"
#include "WorldInstance.h"
#include "WorldInstanceRecord.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldTransferRequest.h"
#include "WorldTransferProfileRegistry.h"
#include "WorldTransferService.h"

struct FrameworkRuntime::Impl
{
	explicit Impl(
		IWorldInstanceFactory& factory,
		const Config& config,
		const IWorldTransferBinding* transferBinding)
		: transferProfileRegistry()
		, executionModelRegistry()
		, worldRegistry(
			factory,
			executionModelRegistry,
			transferProfileRegistry,
			transferBinding)
		, worldManager(worldRegistry)
		, presenceManager()
		, admissionService(worldManager, presenceManager)
		, transferService(worldManager, worldRegistry, admissionService, presenceManager)
		, executionSourceRegistry()
		, graphBuilder()
		, buildPolicy()
		, taskExecutor()
		, executionOps(
			&worldManager,
			&worldRegistry,
			&transferService,
			&admissionService,
			&netIdRegistry,
			&presenceManager)
		, worldScheduler(
			worldManager,
			worldRegistry,
			executionModelRegistry,
			executionSourceRegistry,
			graphBuilder,
			buildPolicy,
			taskExecutor,
			executionOps,
			WorldSchedulerConfig{
				config.maxSelectedWorldsPerFrame
			})
	{
	}

	WorldTransferProfileRegistry transferProfileRegistry;
	WorldExecutionModelRegistry executionModelRegistry;
	NetIdRegistry netIdRegistry;
	WorldRegistry worldRegistry;
	WorldManager worldManager;
	PresenceManager presenceManager;
	WorldAdmissionService admissionService;
	WorldTransferService transferService;
	ExecutionSourceRegistry executionSourceRegistry;
	ExecutionGraphBuilder graphBuilder;
	ExecutionGraphBuildPolicy buildPolicy;
	TaskExecutor taskExecutor;
	ExecutionOps executionOps;
	WorldScheduler worldScheduler;
};

FrameworkRuntime::FrameworkRuntime(Config config)
	: _config(config)
{
}

FrameworkRuntime::~FrameworkRuntime()
{
	Shutdown();
}

bool FrameworkRuntime::Initialize(const BootstrapParams& params)
{
	if (IsInitialized())
	{
		return true;
	}

	if (params.worldFactory == nullptr ||
		params.definitionProvider == nullptr)
	{
		return false;
	}

	_impl = std::make_unique<Impl>(
		*params.worldFactory,
		_config,
		params.transferBinding);

	if (!_impl->taskExecutor.Initialize(_config.executorWorkerCount))
	{
		_impl.reset();
		return false;
	}

	if (!BootstrapDefinitions(params))
	{
		Shutdown();
		return false;
	}

	return true;
}

void FrameworkRuntime::Shutdown() noexcept
{
	if (!_impl)
	{
		return;
	}

	_impl->worldScheduler.Clear();
	_impl->worldRegistry.Clear();
	_impl->executionSourceRegistry.Clear();
	_impl->executionModelRegistry.Clear();
	_impl->transferProfileRegistry.Clear();
	_impl->taskExecutor.Shutdown();
	_impl.reset();
}

bool FrameworkRuntime::IsInitialized() const noexcept
{
	return _impl != nullptr;
}

bool FrameworkRuntime::TickServices(double nowSec, double dtSec)
{
	if (!_impl)
	{
		return false;
	}

	_impl->transferService.Tick(nowSec);
	_impl->admissionService.ExpireReservations(nowSec);
	_impl->worldManager.FlushLifecycle(dtSec);
	_impl->worldManager.CollectDestroyable();
	return true;
}

bool FrameworkRuntime::RunFrame(const FrameParams& params, FrameResult& outResult)
{
	outResult = {};

	if (!_impl)
	{
		outResult.failureReason = WorldSchedulerFailureReason::InternalInvariant;
		return false;
	}

	WorldSchedulerFrameResult schedulerResult{};
	const bool runOk = _impl->worldScheduler.RunFrame(
		WorldSchedulerFrameParams{
			params.frameIndex,
			params.nowSec,
			params.dtSec
		},
		schedulerResult);

	outResult.success = runOk && schedulerResult.success;
	outResult.graphBuilt = schedulerResult.graphBuilt;
	outResult.executed = schedulerResult.executed;
	outResult.selectedWorldCount = schedulerResult.selectedWorldCount;
	outResult.failureReason = schedulerResult.failureReason;
	FrameworkFrameEventHarvester::Harvest(
		_impl->worldManager,
		_impl->worldRegistry,
		_impl->netIdRegistry,
		outResult.events);
	return outResult.success;
}

void FrameworkRuntime::DrainWorldTransferEvents(WorldTransferEventBatch& outEvents)
{
	outEvents.Clear();

	if (!_impl)
	{
		return;
	}

	_impl->transferService.DrainEvents(outEvents);
}

bool FrameworkRuntime::AttachPresenceToWorld(
	SessionId sessionId,
	WorldId worldId,
	double nowSec)
{
	if (!_impl || sessionId == 0 || !worldId.IsValid())
	{
		return false;
	}

	WorldInstanceRecord* targetRecord =
		_impl->worldManager.FindRecord(worldId);
	if (targetRecord == nullptr)
	{
		return false;
	}

	const PresenceRecord* existing =
		_impl->presenceManager.FindBySessionId(sessionId);
	if (existing != nullptr && existing->IsTransfering())
	{
		return false;
	}

	const bool alreadyActiveInTarget =
		existing != nullptr &&
		existing->IsActiveInWorld(worldId);
	const bool moveFromActiveWorld =
		existing != nullptr &&
		existing->state == PresenceStage::Active &&
		existing->currentWorldId.IsValid() &&
		existing->currentWorldId != worldId;

	WorldInstanceRecord* previousRecord = nullptr;
	if (moveFromActiveWorld)
	{
		previousRecord =
			_impl->worldManager.FindRecord(existing->currentWorldId);
		if (previousRecord == nullptr || previousRecord->activePlayers == 0)
		{
			return false;
		}
	}

	if (!_impl->presenceManager.AttachToWorld(sessionId, worldId, nowSec))
	{
		return false;
	}

	if (moveFromActiveWorld)
	{
		--previousRecord->activePlayers;
	}

	if (!alreadyActiveInTarget)
	{
		++targetRecord->activePlayers;
	}

	return true;
}

bool FrameworkRuntime::RemovePresence(SessionId sessionId, double nowSec)
{
	if (!_impl || sessionId == 0)
	{
		return false;
	}

	const PresenceRecord* existing =
		_impl->presenceManager.FindBySessionId(sessionId);
	const bool wasActive =
		existing != nullptr &&
		existing->state == PresenceStage::Active &&
		existing->currentWorldId.IsValid();

	WorldInstanceRecord* previousRecord = nullptr;
	if (wasActive)
	{
		previousRecord =
			_impl->worldManager.FindRecord(existing->currentWorldId);
		if (previousRecord == nullptr || previousRecord->activePlayers == 0)
		{
			return false;
		}
	}

	if (!_impl->presenceManager.RemovePresence(sessionId, nowSec))
	{
		return false;
	}

	if (previousRecord != nullptr)
	{
		--previousRecord->activePlayers;
	}

	return true;
}

TransferId FrameworkRuntime::RequestWorldTransfer(
	std::span<const SessionId> sessionIds,
	WorldId sourceWorldId,
	WorldDefId targetWorldDefId,
	uint64_t instanceKey,
	PartyId partyId,
	bool allowFallback,
	double nowSec)
{
	if (!_impl ||
		sessionIds.empty() ||
		!sourceWorldId.IsValid() ||
		targetWorldDefId == WorldDefId::None)
	{
		return 0;
	}

	WorldTransferRequest request{};
	request.sessionIds.assign(sessionIds.begin(), sessionIds.end());
	request.sourceWorldId = sourceWorldId;
	request.target.targetWorldDefId = targetWorldDefId;
	request.target.instanceKey = instanceKey;
	request.partyId = partyId;
	request.allowFallback = allowFallback;
	request.createdAtSec = nowSec;

	return _impl->transferService.EnqueueRequest(request, nowSec);
}

NetId FrameworkRuntime::AllocateNetId()
{
	if (!_impl)
	{
		return NetId::Invalid();
	}

	return _impl->netIdRegistry.Allocate();
}

void FrameworkRuntime::FreeNetId(NetId netId)
{
	if (!_impl)
	{
		return;
	}

	_impl->netIdRegistry.Free(netId);
}

bool FrameworkRuntime::BindNetEntity(NetId netId, WorldId worldId, Entity entity)
{
	if (!_impl)
	{
		return false;
	}

	return _impl->netIdRegistry.BindEntity(netId, worldId, entity);
}

bool FrameworkRuntime::UnbindNetEntity(NetId netId)
{
	if (!_impl)
	{
		return false;
	}

	return _impl->netIdRegistry.UnbindEntity(netId);
}

NetBindingLocation FrameworkRuntime::FindNetBinding(NetId netId) const
{
	if (!_impl)
	{
		return {};
	}

	return _impl->netIdRegistry.FindLocation(netId);
}

NetId FrameworkRuntime::FindNetId(WorldId worldId, Entity entity) const
{
	if (!_impl)
	{
		return NetId::Invalid();
	}

	return _impl->netIdRegistry.FindNetId(worldId, entity);
}

NetId FrameworkRuntime::BindEntityToNet(WorldId worldId, Entity entity)
{
	if (!_impl || !worldId.IsValid() || entity.IsNull())
	{
		return NetId::Invalid();
	}

	NetId netId = _impl->netIdRegistry.FindNetId(worldId, entity);
	if (netId.IsValid())
	{
		return netId;
	}

	netId = _impl->netIdRegistry.Allocate();
	if (!netId.IsValid())
	{
		return NetId::Invalid();
	}

	if (!_impl->netIdRegistry.BindEntity(netId, worldId, entity))
	{
		_impl->netIdRegistry.Free(netId);
		return NetId::Invalid();
	}

	return netId;
}

bool FrameworkRuntime::IsNetIdAlive(NetId netId) const
{
	if (!_impl)
	{
		return false;
	}

	return _impl->netIdRegistry.IsAlive(netId);
}

WorldId FrameworkRuntime::ResolveOrCreateWorld(WorldDefId defId, uint64_t instanceKey)
{
	if (!_impl)
	{
		return WorldId::Invalid();
	}

	return _impl->worldManager.ResolveOrCreate(defId, instanceKey);
}

WorldId FrameworkRuntime::RegisterPreCreatedWorld(WorldDefId defId, uint64_t instanceKey)
{
	if (!_impl)
	{
		return WorldId::Invalid();
	}

	return _impl->worldManager.RegisterPreCreatedWorld(defId, instanceKey);
}

bool FrameworkRuntime::InitializeWorld(WorldId worldId)
{
	WorldInstance* world = FindWorld(worldId);
	if (world == nullptr)
	{
		return false;
	}

	return world->Initialize();
}

void FrameworkRuntime::RequestCloseWorld(WorldId worldId)
{
	if (!_impl)
	{
		return;
	}

	_impl->worldManager.RequestClose(worldId);
}

void FrameworkRuntime::CollectDestroyableWorlds()
{
	if (!_impl)
	{
		return;
	}

	_impl->worldManager.CollectDestroyable();
}

WorldInstance* FrameworkRuntime::FindWorld(WorldId worldId)
{
	if (!_impl)
	{
		return nullptr;
	}

	return _impl->worldRegistry.FindWorld(worldId);
}

const WorldInstance* FrameworkRuntime::FindWorld(WorldId worldId) const
{
	if (!_impl)
	{
		return nullptr;
	}

	return _impl->worldRegistry.FindWorld(worldId);
}

WorldInstanceRecord* FrameworkRuntime::FindWorldRecord(WorldId worldId)
{
	if (!_impl)
	{
		return nullptr;
	}

	return _impl->worldManager.FindRecord(worldId);
}

const WorldInstanceRecord* FrameworkRuntime::FindWorldRecord(WorldId worldId) const
{
	if (!_impl)
	{
		return nullptr;
	}

	return _impl->worldManager.FindRecord(worldId);
}

std::span<const WorldId> FrameworkRuntime::GetRunnableWorldIds()
{
	if (!_impl)
	{
		return {};
	}

	return _impl->worldManager.GetRunnableWorldIds();
}

bool FrameworkRuntime::BootstrapDefinitions(const BootstrapParams& params)
{
	if (!_impl || params.definitionProvider == nullptr)
	{
		return false;
	}

	return BootstrapWorldDefinitions(
		*params.definitionProvider,
		_impl->executionSourceRegistry,
		_impl->executionModelRegistry,
		_impl->transferProfileRegistry,
		_impl->worldRegistry);
}
