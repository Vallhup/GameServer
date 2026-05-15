#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Entity.h"
#include "NetIdRegistry.h"
#include "Session.h"
#include "TaskExecutorDiagnostics.h"
#include "WorldId.h"
#include "WorldContentIds.h"
#include "WorldIds.h"
#include "WorldScheduler.h"
#include "WorldTransferEvents.h"

class IWorldInstanceFactory;
class IWorldDefinitionProvider;
class IWorldTransferBinding;
class WorldRuntime;
class WorldInstance;
struct WorldInstanceRecord;
struct IExecutorIOSink;
struct IIOBackend;
struct INetworkBackend;
class DynamicTaskTypeRegistry;
class ExecutionSourceRegistry;
class IDynamicTaskScopeResolver;
enum class SystemPhase : uint8_t;
enum class ExecPhase : uint8_t;

class FrameworkRuntime final {
public:
	struct Config
	{
		uint32_t executorWorkerCount{ 4 };
		uint32_t maxSelectedWorldsPerFrame{ 0 };
		TaskExecutorDiagnosticsConfig executorDiagnostics{};
	};

	struct BootstrapParams
	{
		IWorldInstanceFactory* worldFactory{ nullptr };
		const IWorldDefinitionProvider* definitionProvider{ nullptr };
		const IWorldTransferBinding* transferBinding{ nullptr };
	};

	struct FrameParams
	{
		uint64_t frameIndex{ 0 };
		double nowSec{ 0.0 };
		double dtSec{ 0.0 };
	};

	struct FrameResult
	{
		struct EntitySpawnEvent
		{
			WorldId worldId{ WorldId::Invalid() };
			Entity entity{ Entity::Null() };
			NetId netId{ NetId::Invalid() };
		};

		struct EntityDespawnEvent
		{
			WorldId worldId{ WorldId::Invalid() };
			Entity entity{ Entity::Null() };
			NetId netId{ NetId::Invalid() };
		};

		struct CombatImpactEvent
		{
			WorldId worldId{ WorldId::Invalid() };
			NetId attackerNetId{ NetId::Invalid() };
			NetId victimNetId{ NetId::Invalid() };
			uint32_t resultType{ 0 };
			float impactX{ 0.0f };
			float impactY{ 0.0f };
			float impactZ{ 0.0f };
			float dirX{ 0.0f };
			float dirY{ 0.0f };
			float dirZ{ -1.0f };
		};

		struct FrameEvents
		{
			std::vector<EntitySpawnEvent> spawns;
			std::vector<EntityDespawnEvent> despawns;
			std::vector<CombatImpactEvent> combatImpacts;

			void Clear() noexcept
			{
				spawns.clear();
				despawns.clear();
				combatImpacts.clear();
			}
		};

		bool success{ false };
		bool graphBuilt{ false };
		bool executed{ false };
		uint32_t selectedWorldCount{ 0 };
		WorldSchedulerFailureReason failureReason{
			WorldSchedulerFailureReason::None
		};
		TaskExecutorFrameDiagnostics executorDiagnostics;
		FrameEvents events;
	};

public:
	explicit FrameworkRuntime(Config config = {});
	~FrameworkRuntime();

	FrameworkRuntime(const FrameworkRuntime&) = delete;
	FrameworkRuntime& operator=(const FrameworkRuntime&) = delete;

	bool Initialize(const BootstrapParams& params);
	void Shutdown() noexcept;

	bool IsInitialized() const noexcept;

	bool TickServices(double nowSec, double dtSec);
	bool RunFrame(const FrameParams& params, FrameResult& outResult);
	void DrainWorldTransferEvents(WorldTransferEventBatch& outEvents);

	// IOCP 네트워크 백엔드 연동 — TaskExecutor에 위임
	IExecutorIOSink& GetIOSink() noexcept;
	void SetNetworkBackend(INetworkBackend* backend) noexcept;
	void RegisterIOBackend(IIOBackend* backend) noexcept;
	void UnregisterIOBackend(IIOBackend* backend) noexcept;

	// DynamicTask 패킷 핸들러 등록용
	DynamicTaskTypeRegistry& GetDynamicTaskTypeRegistry() noexcept;
	ExecutionSourceRegistry& GetExecutionSourceRegistry() noexcept;
	void SetDynamicTaskScopeResolver(
		IDynamicTaskScopeResolver* resolver) noexcept;

	bool AttachPresenceToWorld(
		SessionId sessionId,
		WorldId worldId,
		double nowSec);

	bool RemovePresence(SessionId sessionId, double nowSec);

	TransferId RequestWorldTransfer(
		std::span<const SessionId> sessionIds,
		WorldId sourceWorldId,
		WorldDefId targetWorldDefId,
		uint64_t instanceKey,
		PartyId partyId,
		bool allowFallback,
		double nowSec);

	NetId AllocateNetId();
	void FreeNetId(NetId netId);

	bool BindNetEntity(NetId netId, WorldId worldId, Entity entity);
	bool UnbindNetEntity(NetId netId);

	NetBindingLocation FindNetBinding(NetId netId) const;
	NetId FindNetId(WorldId worldId, Entity entity) const;

	// Allocate + Bind 를 atomic 하게 수행. 실패 시 자동 free.
	// 이미 같은 (worldId, entity) 로 bound 된 NetId 가 있으면 그것을 반환.
	// 실패 시 NetId::Invalid() 반환.
	NetId BindEntityToNet(WorldId worldId, Entity entity);
	bool IsNetIdAlive(NetId netId) const;

	WorldId ResolveOrCreateWorld(WorldDefId defId, uint64_t instanceKey);
	WorldId RegisterPreCreatedWorld(WorldDefId defId, uint64_t instanceKey);

	bool InitializeWorld(WorldId worldId);
	void RequestCloseWorld(WorldId worldId);
	void CollectDestroyableWorlds();

	bool BindRuntimeSystems(WorldRuntime& runtime, ExecPhase execPhase);

	WorldInstance* FindWorld(WorldId worldId);
	const WorldInstance* FindWorld(WorldId worldId) const;

	WorldInstanceRecord* FindWorldRecord(WorldId worldId);
	const WorldInstanceRecord* FindWorldRecord(WorldId worldId) const;

	std::span<const WorldId> GetRunnableWorldIds();

private:
	struct Impl;

	bool BootstrapDefinitions(const BootstrapParams& params);

private:
	Config _config;
	std::unique_ptr<Impl> _impl;
};
