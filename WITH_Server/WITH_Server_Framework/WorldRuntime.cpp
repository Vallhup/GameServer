#include "pch.h"
#include "WorldRuntime.h"

#include <unordered_set>

#include "IWorldTransferBinding.h"
#include "WorldTransferContext.h"
#include "WorldTransferProfile.h"
#include "WorldDef.h"

namespace
{
	thread_local std::vector<WorldLifecycleCommand>* g_activeLifecycleSignalStaging{ nullptr };

	class ScopedLifecycleSignalStaging final {
	public:
		explicit ScopedLifecycleSignalStaging(std::vector<WorldLifecycleCommand>& staging)
			: _previous(g_activeLifecycleSignalStaging)
		{
			g_activeLifecycleSignalStaging = &staging;
		}

		~ScopedLifecycleSignalStaging()
		{
			g_activeLifecycleSignalStaging = _previous;
		}

	private:
		std::vector<WorldLifecycleCommand>* _previous{ nullptr };
	};

	const WorldTransferContext* TryGetConcreteTransferContext(const ITransferContext& context) noexcept
	{
		return dynamic_cast<const WorldTransferContext*>(&context);
	}
}

WorldRuntime::WorldRuntime(WorldRuntimeCreateParams params)
	: _def(params.def)
	, _executionModel(params.executionModel)
	, _transferBinding(params.transferBinding)
	, _transferProfile(params.transferProfile)
{
}

bool WorldRuntime::Initialize()
{
	if (IsShutdown())
		return false;

	if (IsInitialized())
		return true;

	if (IsFaulted())
		return false;

	if (_def == nullptr || _executionModel == nullptr)
	{
		MarkFault(
			WorldRuntimeFaultCode::InitFailed,
			"WorldRuntime requires valid WorldDef and WorldExecutionModel.");
		return false;
	}

	_lifecycleState = WorldRuntimeLifecycleState::Running;
	_commitState = WorldRuntimeCommitState::NotCommitted;
	_lifecycleFlushState = WorldRuntimeLifecycleFlushState::NotFlushed;
	_hasBegunAnyFrame = false;
	_frameOpen = false;
	_frameIndex = 0;
	_lastNowSec = 0.0;
	_lastDtSec = 0.0;

	// NavMesh 로딩 — WorldDef.map.navMesh가 설정된 경우에만 수행
	// 실패해도 초기화를 중단하지 않는다. 해당 월드는 fallback 경로로 동작한다.
	_navMeshRuntime.reset();
	_navProfile = nullptr;

	if (_def->map.navMesh.has_value())
	{
		_navMeshRuntime = std::make_unique<NavMeshRuntime>();
		if (!_navMeshRuntime->LoadFromFile(_def->map.navMesh->navMeshBinPath))
		{
			// 로딩 실패 — NavMesh 없이 계속 실행
			// TODO: 로깅 인프라 연결 후 경고 기록
			_navMeshRuntime.reset();
		}
	}

	if (_navMeshRuntime && _def->map.navigationProfile.has_value())
		_navProfile = &(*_def->map.navigationProfile);

	return true;
}

void WorldRuntime::Shutdown()
{
	if (IsShutdown())
		return;

	_frameCommands.Clear();
	_worldCommands.Clear();
	_frameWorldCommands.clear();
	_lifecycleCommands.Clear();
	_lifecycleOutbox.clear();
	_systems.Clear();
	_ecs.Clear();

	_commitState = WorldRuntimeCommitState::NotCommitted;
	_lifecycleFlushState = WorldRuntimeLifecycleFlushState::NotFlushed;
	_lifecycleState = WorldRuntimeLifecycleState::Shutdown;
	_storagesFixed = false;
	_hasBegunAnyFrame = false;
	_frameOpen = false;
	_frameIndex = 0;
	_lastNowSec = 0.0;
	_lastDtSec = 0.0;
}

bool WorldRuntime::BeginFrame(uint64_t frameIndex, double nowSec, double dtSec)
{
	if (!CanBeginFrame())
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"BeginFrame is not allowed in the current runtime state.");
		return false;
	}

	if (_hasBegunAnyFrame && frameIndex <= _frameIndex)
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"BeginFrame requires strictly increasing frameIndex.");
		return false;
	}

	_frameIndex = frameIndex;
	_lastNowSec = nowSec;
	_lastDtSec = dtSec;
	_commitState = WorldRuntimeCommitState::NotCommitted;
	_lifecycleFlushState = WorldRuntimeLifecycleFlushState::NotFlushed;
	_worldCommands.DrainTo(_frameWorldCommands);
	_hasBegunAnyFrame = true;
	_frameOpen = true;
	return true;
}

bool WorldRuntime::FlushFrameCommands()
{
	if (!CanFlushFrameCommands())
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"FlushFrameCommands is not allowed in the current runtime state.");
		if (_frameOpen && _commitState == WorldRuntimeCommitState::NotCommitted)
		{
			_commitState = WorldRuntimeCommitState::CommitFailed;
		}
		return false;
	}

	std::vector<WorldLifecycleCommand> stagedSignals;
	{
		ScopedLifecycleSignalStaging stagingScope(stagedSignals);
		_frameCommands.Commit(*this);
	}

	if (IsFaulted())
	{
		_commitState = WorldRuntimeCommitState::CommitFailed;
		return false;
	}

	for (WorldLifecycleCommand& command : stagedSignals)
	{
		_lifecycleCommands.Enqueue(std::move(command));
	}

	_commitState = WorldRuntimeCommitState::CommitSucceeded;
	return true;
}

bool WorldRuntime::FlushLifecycleCommands()
{
	if (!CanFlushLifecycleCommands())
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"FlushLifecycleCommands is not allowed in the current runtime state.");
		return false;
	}

	_lifecycleOutbox.clear();
	_frameWorldCommands.clear();
	_lifecycleCommands.DrainTo(_lifecycleOutbox);
	_lifecycleFlushState = WorldRuntimeLifecycleFlushState::Flushed;
	_frameOpen = false;
	return true;
}

bool WorldRuntime::EnqueueWorldCommand(WorldCommand command)
{
	if (!CanAcceptWorldCommand())
	{
		return false;
	}

	return _worldCommands.Enqueue(std::move(command));
}

// WorldNavMeshServiceAdapter: WorldRuntime이 소유한 NavMeshRuntime을
// INavMeshProvider 인터페이스로 노출하는 경량 어댑터.
// ExecuteSystems() 스택 내에서만 생존하므로 포인터 수명이 안전하다.
struct WorldNavMeshServiceAdapter final : INavMeshProvider
{
	const NavMeshRuntime*       runtime{ nullptr };
	const NavigationProfileDef* profile{ nullptr };

	const NavMeshRuntime* GetNavMeshRuntime() const noexcept override
	{
		return runtime;
	}

	const NavigationProfileDef* GetNavigationProfile() const noexcept override
	{
		return profile;
	}
};

bool WorldRuntime::ExecuteSystems(
	SystemPhase phase,
	WorldSystemServices services)
{
	if (_lifecycleState != WorldRuntimeLifecycleState::Running ||
		IsShutdown() ||
		IsFaulted() ||
		!_frameOpen ||
		_commitState != WorldRuntimeCommitState::NotCommitted)
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"ExecuteSystems is not allowed in the current runtime state.");
		return false;
	}

	// 호출자가 navMeshProvider를 설정하지 않은 경우 WorldRuntime 소유 NavMesh를 자동 주입
	WorldNavMeshServiceAdapter navAdapter;
	if (!services.navMeshProvider && _navMeshRuntime && _navMeshRuntime->IsReady())
	{
		navAdapter.runtime = _navMeshRuntime.get();
		navAdapter.profile = _navProfile;
		services.navMeshProvider = &navAdapter;
	}

	SystemContext context{
		*this,
		MakeView(),
		_lastDtSec,
		services
	};

	for (System* system : _systems.GetSystems(phase))
	{
		if (system == nullptr)
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"ExecuteSystems encountered a null system.");
			return false;
		}

		system->Execute(context);
		if (IsFaulted())
		{
			return false;
		}
	}

	return true;
}

bool WorldRuntime::BuildTransferContext(
	const std::vector<uint32_t>& sessionIds,
	std::unique_ptr<ITransferContext>& outContext)
{
	outContext.reset();

	if (_lifecycleState != WorldRuntimeLifecycleState::Running || IsShutdown() || IsFaulted())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferBuildFailed,
			"BuildTransferContext is not allowed in the current runtime state.");
		return false;
	}

	if (_transferProfile == nullptr)
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferBuildFailed,
			"BuildTransferContext requires a resolved WorldTransferProfile.");
		return false;
	}

	if (_transferBinding == nullptr)
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferBuildFailed,
			"BuildTransferContext requires a valid IWorldTransferBinding.");
		return false;
	}

	if (sessionIds.empty())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferBuildFailed,
			"BuildTransferContext requires at least one sessionId.");
		return false;
	}

	ECSView sourceView = MakeView();
	auto context = std::make_unique<WorldTransferContext>();
	auto& contextSessionIds = context->MutableSessionIds();
	auto& entities = context->MutableEntities();

	contextSessionIds.reserve(sessionIds.size());
	entities.reserve(sessionIds.size());

	std::unordered_set<uint32_t> seenSessionIds;
	seenSessionIds.reserve(sessionIds.size());

	for (uint32_t sessionId : sessionIds)
	{
		if (!seenSessionIds.insert(sessionId).second)
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferBuildFailed,
				"BuildTransferContext received duplicate sessionId.");
			return false;
		}

		Entity rootEntity = Entity::Null();
		NetId rootNetId = NetId::Invalid();
		if (!_transferBinding->TryResolveRootEntity(sessionId, rootEntity, rootNetId) ||
			rootEntity.IsNull() ||
			!rootNetId.IsValid() ||
			!sourceView.IsAlive(rootEntity))
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferBuildFailed,
				"BuildTransferContext failed to resolve a live root entity.");
			return false;
		}

		TransferEntitySnapshot entitySnapshot;
		entitySnapshot.sessionId = sessionId;
		entitySnapshot.sourceEntity = rootEntity;
		entitySnapshot.netId = rootNetId;

		for (const IWorldTransferSerializer* serializer : _transferProfile->Serializers())
		{
			if (serializer == nullptr)
			{
				MarkFault(
					WorldRuntimeFaultCode::TransferBuildFailed,
					"BuildTransferContext encountered a null transfer serializer.");
				return false;
			}

			TransferPayloadSnapshot payloadSnapshot;
			payloadSnapshot.serializerId = serializer->GetSerializerId();

			const WorldTransferExportContext exportContext{
				.sourceView = sourceView,
				.sessionId = sessionId,
				.sourceEntity = rootEntity,
				.netId = rootNetId
			};
			if (!serializer->Export(exportContext, payloadSnapshot.bytes))
			{
				MarkFault(
					WorldRuntimeFaultCode::TransferBuildFailed,
					"BuildTransferContext failed while exporting a transfer payload snapshot.");
				return false;
			}

			entitySnapshot.payloads.push_back(std::move(payloadSnapshot));
		}

		contextSessionIds.push_back(sessionId);
		entities.push_back(std::move(entitySnapshot));
	}

	outContext = std::move(context);
	return true;
}

bool WorldRuntime::ImportTransferContext(
	const ITransferContext& context,
	std::vector<ImportedTransferEntity>& outImportedEntities)
{
	outImportedEntities.clear();

	if (_lifecycleState != WorldRuntimeLifecycleState::Running || IsShutdown() || IsFaulted())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferImportFailed,
			"ImportTransferContext is not allowed in the current runtime state.");
		return false;
	}

	if (_transferProfile == nullptr)
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferImportFailed,
			"ImportTransferContext requires a resolved WorldTransferProfile.");
		return false;
	}

	const WorldTransferContext* concreteContext = TryGetConcreteTransferContext(context);
	if (concreteContext == nullptr)
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferImportFailed,
			"ImportTransferContext requires a WorldTransferContext payload.");
		return false;
	}

	const std::span<const uint32_t> sessionIds = concreteContext->SessionIds();
	const std::span<const TransferEntitySnapshot> entities = concreteContext->Entities();

	if (sessionIds.empty())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferImportFailed,
			"ImportTransferContext requires at least one sessionId.");
		return false;
	}

	if (sessionIds.size() != entities.size())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferImportFailed,
			"ImportTransferContext requires matching sessionId and entity snapshot counts.");
		return false;
	}

	std::unordered_set<uint32_t> seenSessionIds;
	seenSessionIds.reserve(sessionIds.size());

	for (size_t index = 0; index < entities.size(); ++index)
	{
		const uint32_t expectedSessionId = sessionIds[index];
		const TransferEntitySnapshot& entitySnapshot = entities[index];

		if (entitySnapshot.sessionId != expectedSessionId)
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferImportFailed,
				"ImportTransferContext requires ordered sessionId/entity snapshot alignment.");
			return false;
		}

		if (!seenSessionIds.insert(expectedSessionId).second)
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferImportFailed,
				"ImportTransferContext received duplicate sessionId.");
			return false;
		}

		if (!entitySnapshot.netId.IsValid())
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferImportFailed,
				"ImportTransferContext requires a valid transfer net id.");
			return false;
		}

		std::unordered_set<WorldTransferSerializerId> seenSerializerIds;
		seenSerializerIds.reserve(entitySnapshot.payloads.size());

		for (const TransferPayloadSnapshot& payloadSnapshot : entitySnapshot.payloads)
		{
			if (!seenSerializerIds.insert(payloadSnapshot.serializerId).second)
			{
				MarkFault(
					WorldRuntimeFaultCode::TransferImportFailed,
					"ImportTransferContext received duplicate payload snapshots for one entity.");
				return false;
			}

			if (_transferProfile->Find(payloadSnapshot.serializerId) == nullptr)
			{
				MarkFault(
					WorldRuntimeFaultCode::TransferImportFailed,
					"ImportTransferContext encountered an unknown transfer serializer type.");
				return false;
			}
		}
	}

	outImportedEntities.reserve(sessionIds.size());

	for (const TransferEntitySnapshot& entitySnapshot : entities)
	{
		const uint32_t sessionId = entitySnapshot.sessionId;
		Entity targetEntity = ReserveEntity();
		if (targetEntity.IsNull() || IsFaulted())
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferImportFailed,
				"ImportTransferContext failed to reserve a target entity.");
			return false;
		}

		for (const TransferPayloadSnapshot& payloadSnapshot : entitySnapshot.payloads)
		{
			const IWorldTransferSerializer* serializer =
				_transferProfile->Find(payloadSnapshot.serializerId);
			if (serializer == nullptr)
			{
				MarkFault(
					WorldRuntimeFaultCode::TransferImportFailed,
					"ImportTransferContext encountered an unknown transfer serializer type.");
				return false;
			}

			const WorldTransferImportContext importContext{
				.targetRuntime = *this,
				.sessionId = sessionId,
				.sourceEntity = entitySnapshot.sourceEntity,
				.targetEntity = targetEntity,
				.netId = entitySnapshot.netId
			};
			if (!serializer->Import(importContext, payloadSnapshot.bytes) || IsFaulted())
			{
				MarkFault(
					WorldRuntimeFaultCode::TransferImportFailed,
					"ImportTransferContext failed while importing a transfer payload snapshot.");
				return false;
			}
		}

		EnqueueLifecycle(WorldLifecycleCommand::TransferImported(sessionId, targetEntity));
		if (IsFaulted())
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferImportFailed,
				"ImportTransferContext failed while enqueueing lifecycle signals.");
			return false;
		}

		ImportedTransferEntity imported{};
		imported.sessionId = sessionId;
		imported.sourceEntity = entitySnapshot.sourceEntity;
		imported.targetEntity = targetEntity;
		imported.netId = entitySnapshot.netId;
		outImportedEntities.push_back(imported);
	}

	return true;
}

bool WorldRuntime::ReleaseTransferContext(
	const ITransferContext& context,
	std::vector<uint32_t>& outReleasedSessionIds)
{
	outReleasedSessionIds.clear();

	if (_lifecycleState != WorldRuntimeLifecycleState::Running || IsShutdown() || IsFaulted())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferReleaseFailed,
			"ReleaseTransferContext is not allowed in the current runtime state.");
		return false;
	}

	const WorldTransferContext* concreteContext = TryGetConcreteTransferContext(context);
	if (concreteContext == nullptr)
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferReleaseFailed,
			"ReleaseTransferContext requires a WorldTransferContext payload.");
		return false;
	}

	const std::span<const uint32_t> sessionIds = concreteContext->SessionIds();
	const std::span<const TransferEntitySnapshot> entities = concreteContext->Entities();

	if (sessionIds.empty() || sessionIds.size() != entities.size())
	{
		MarkFault(
			WorldRuntimeFaultCode::TransferReleaseFailed,
			"ReleaseTransferContext requires matching sessionId and entity snapshot counts.");
		return false;
	}

	ECSView sourceView = MakeView();
	std::unordered_set<uint32_t> seenSessionIds;
	seenSessionIds.reserve(sessionIds.size());
	outReleasedSessionIds.reserve(sessionIds.size());

	for (size_t index = 0; index < entities.size(); ++index)
	{
		const uint32_t expectedSessionId = sessionIds[index];
		const TransferEntitySnapshot& entitySnapshot = entities[index];

		if (entitySnapshot.sessionId != expectedSessionId)
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferReleaseFailed,
				"ReleaseTransferContext requires ordered sessionId/entity snapshot alignment.");
			return false;
		}

		if (!seenSessionIds.insert(expectedSessionId).second)
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferReleaseFailed,
				"ReleaseTransferContext received duplicate sessionId.");
			return false;
		}

		if (entitySnapshot.sourceEntity.IsNull() || !sourceView.IsAlive(entitySnapshot.sourceEntity))
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferReleaseFailed,
				"ReleaseTransferContext requires a live source entity.");
			return false;
		}
	}

	for (const TransferEntitySnapshot& entitySnapshot : entities)
	{
		DeferredDestroyEntity(entitySnapshot.sourceEntity);
		if (IsFaulted())
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferReleaseFailed,
				"ReleaseTransferContext failed while scheduling source cleanup.");
			return false;
		}

		EnqueueLifecycle(
			WorldLifecycleCommand::TransferReleased(
				entitySnapshot.sessionId,
				entitySnapshot.sourceEntity));
		if (IsFaulted())
		{
			MarkFault(
				WorldRuntimeFaultCode::TransferReleaseFailed,
				"ReleaseTransferContext failed while enqueueing lifecycle signals.");
			return false;
		}

		outReleasedSessionIds.push_back(entitySnapshot.sessionId);
	}

	return true;
}

bool WorldRuntime::RollbackImportedTransferContext(
	const ITransferContext& context,
	const std::vector<ImportedTransferEntity>& importedEntities)
{
	(void)context;
	(void)importedEntities;

	MarkFault(
		WorldRuntimeFaultCode::TransferRollbackFailed,
		"RollbackImportedTransferContext is not implemented yet.");
	return false;
}

void WorldRuntime::EnsureStorageRegistrationAllowed() const
{
	if (IsShutdown())
		throw std::logic_error("WorldRuntime is shutdown.");

	if (_storagesFixed)
		throw std::logic_error("WorldRuntime storages are already fixed.");
}

void WorldRuntime::MarkFault(WorldRuntimeFaultCode code, const char* message)
{
	if (_fault.HasError())
		return;

	_fault.code = code;
	if (message != nullptr)
	{
		_fault.message = message;
	}
}

bool WorldRuntime::CanBeginFrame() const
{
	return
		_lifecycleState == WorldRuntimeLifecycleState::Running &&
		!IsFaulted() &&
		!IsShutdown() &&
		!_frameOpen;
}

bool WorldRuntime::CanFlushFrameCommands() const
{
	return
		_lifecycleState == WorldRuntimeLifecycleState::Running &&
		!IsFaulted() &&
		!IsShutdown() &&
		_frameOpen &&
		_commitState == WorldRuntimeCommitState::NotCommitted;
}

bool WorldRuntime::CanFlushLifecycleCommands() const
{
	return
		_lifecycleState == WorldRuntimeLifecycleState::Running &&
		!IsShutdown() &&
		_frameOpen &&
		_commitState != WorldRuntimeCommitState::NotCommitted &&
		_lifecycleFlushState == WorldRuntimeLifecycleFlushState::NotFlushed;
}

bool WorldRuntime::CanAcceptStructuralMutation() const
{
	return
		_lifecycleState == WorldRuntimeLifecycleState::Running &&
		!IsFaulted() &&
		!IsShutdown();
}

bool WorldRuntime::CanAcceptLifecycleSignal() const
{
	return
		_lifecycleState == WorldRuntimeLifecycleState::Running &&
		!IsFaulted() &&
		!IsShutdown();
}

bool WorldRuntime::CanAcceptWorldCommand() const
{
	return
		_lifecycleState == WorldRuntimeLifecycleState::Running &&
		!IsFaulted() &&
		!IsShutdown();
}

bool WorldRuntime::MaterializeReservedEntityImmediate(Entity reserved)
{
	if (!_ecs.MaterializeReservedEntityImmediate(reserved))
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"Failed to materialize reserved entity.");
		return false;
	}

	if (g_activeLifecycleSignalStaging != nullptr)
	{
		g_activeLifecycleSignalStaging->push_back(
			WorldLifecycleCommand::EntitySpawned(reserved));
	}

	return true;
}

bool WorldRuntime::DestroyEntityImmediate(Entity e)
{
	if (!_ecs.DestroyEntityImmediate(e))
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"Failed to destroy entity.");
		return false;
	}

	if (g_activeLifecycleSignalStaging != nullptr)
	{
		g_activeLifecycleSignalStaging->push_back(
			WorldLifecycleCommand::EntityDespawned(e));
	}

	return true;
}

Entity WorldRuntime::ReserveEntity()
{
	if (!CanAcceptStructuralMutation())
	{
		MarkFault(
			WorldRuntimeFaultCode::InvalidOperation,
			"ReserveEntity is not allowed in the current runtime state.");
		return Entity::Null();
	}

	Entity reserved = _ecs._entityMng.Reserve();
	_frameCommands.Enqueue(
		[entity = reserved](WorldRuntime& rt)
		{
			(void)rt.MaterializeReservedEntityImmediate(entity);
		});

	return reserved;
}
