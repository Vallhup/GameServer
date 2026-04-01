#include "pch.h"
#include "FrameworkRuntime.h"

#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionOps.h"
#include "ExecutionSourceTypes.h"
#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"
#include "NetIdRegistry.h"
#include "PresenceManager.h"
#include "RepComponent.h"
#include "TaskExecutor.h"
#include "WorldAdmissionService.h"
#include "WorldDefinitionBootstrap.h"
#include "WorldExecutionModelTypes.h"
#include "WorldInstance.h"
#include "WorldInstanceRecord.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldTransferProfileRegistry.h"
#include "WorldTransferService.h"

struct FrameworkRuntime::Impl
{
	explicit Impl(
		IWorldInstanceFactory& factory,
		const Config& config)
		: transferProfileRegistry()
		, executionModelRegistry()
		, worldRegistry(factory, executionModelRegistry, transferProfileRegistry)
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

	_impl = std::make_unique<Impl>(*params.worldFactory, _config);

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
	HarvestFrameEvents(outResult.events);
	return outResult.success;
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
		_impl->worldRegistry);
}

void FrameworkRuntime::HarvestFrameEvents(FrameResult::FrameEvents& outEvents)
{
	outEvents.Clear();

	if (!_impl)
	{
		return;
	}

	for (WorldId worldId : _impl->worldManager.GetRunnableWorldIds())
	{
		WorldInstance* world = _impl->worldRegistry.FindWorld(worldId);
		if (world == nullptr)
		{
			continue;
		}

		WorldRuntime& runtime = world->GetRuntime();
		const ECSView view = runtime.MakeView();
		for (const WorldLifecycleCommand& command : runtime.LifecycleOutbox())
		{
			switch (command.kind) {
			case WorldLifecycleCommandKind::EntitySpawned:
			{
				if (!view.HasComponent<ReplicatedTag>(command.entity))
				{
					break;
				}

				NetId netId = _impl->netIdRegistry.FindNetId(worldId, command.entity);
				if (!netId.IsValid())
				{
					netId = _impl->netIdRegistry.Allocate();
					if (!netId.IsValid())
					{
						break;
					}

					if (!_impl->netIdRegistry.BindEntity(netId, worldId, command.entity))
					{
						_impl->netIdRegistry.Free(netId);
						break;
					}
				}

				outEvents.spawns.push_back(
					FrameResult::EntitySpawnEvent{
						worldId,
						command.entity,
						netId
					});
				break;
			}

			case WorldLifecycleCommandKind::EntityDespawned:
			{
				const NetId netId = _impl->netIdRegistry.FindNetId(worldId, command.entity);
				if (!netId.IsValid())
				{
					break;
				}

				outEvents.despawns.push_back(
					FrameResult::EntityDespawnEvent{
						worldId,
						command.entity,
						netId
					});

				(void)_impl->netIdRegistry.UnbindEntity(netId);
				_impl->netIdRegistry.Free(netId);
				break;
			}

			default:
				break;
			}
		}

		runtime.ClearLifecycleOutbox();
	}
}
