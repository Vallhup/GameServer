#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Entity.h"
#include "NetIdRegistry.h"
#include "WorldId.h"
#include "WorldContentIds.h"
#include "WorldScheduler.h"

class IWorldInstanceFactory;
class IWorldDefinitionProvider;
class WorldInstance;
struct WorldInstanceRecord;

class FrameworkRuntime final {
public:
	struct Config
	{
		uint32_t executorWorkerCount{ 4 };
		uint32_t maxSelectedWorldsPerFrame{ 0 };
	};

	struct BootstrapParams
	{
		IWorldInstanceFactory* worldFactory{ nullptr };
		const IWorldDefinitionProvider* definitionProvider{ nullptr };
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

		struct FrameEvents
		{
			std::vector<EntitySpawnEvent> spawns;
			std::vector<EntityDespawnEvent> despawns;

			void Clear() noexcept
			{
				spawns.clear();
				despawns.clear();
			}
		};

		bool success{ false };
		bool graphBuilt{ false };
		bool executed{ false };
		uint32_t selectedWorldCount{ 0 };
		WorldSchedulerFailureReason failureReason{
			WorldSchedulerFailureReason::None
		};
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

	NetId AllocateNetId();
	void FreeNetId(NetId netId);

	bool BindNetEntity(NetId netId, WorldId worldId, Entity entity);
	bool UnbindNetEntity(NetId netId);

	NetBindingLocation FindNetBinding(NetId netId) const;
	NetId FindNetId(WorldId worldId, Entity entity) const;
	bool IsNetIdAlive(NetId netId) const;

	WorldId ResolveOrCreateWorld(WorldDefId defId, uint64_t instanceKey);
	WorldId RegisterPreCreatedWorld(WorldDefId defId, uint64_t instanceKey);

	bool InitializeWorld(WorldId worldId);
	void RequestCloseWorld(WorldId worldId);
	void CollectDestroyableWorlds();

	WorldInstance* FindWorld(WorldId worldId);
	const WorldInstance* FindWorld(WorldId worldId) const;

	WorldInstanceRecord* FindWorldRecord(WorldId worldId);
	const WorldInstanceRecord* FindWorldRecord(WorldId worldId) const;

	std::span<const WorldId> GetRunnableWorldIds();

private:
	struct Impl;

	bool BootstrapDefinitions(const BootstrapParams& params);
	void HarvestFrameEvents(FrameResult::FrameEvents& outEvents);

private:
	Config _config;
	std::unique_ptr<Impl> _impl;
};
