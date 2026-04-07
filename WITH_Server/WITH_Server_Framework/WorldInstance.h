#pragma once

#include <memory>

#include "WorldRuntime.h"
#include "WorldContentIds.h"
#include "WorldId.h"
#include "WorldExecutionModelTypes.h"

struct WorldDef;
class IWorldTransferBinding;
class WorldTransferProfile;

class IWorldInstanceImpl
{
public:
	virtual ~IWorldInstanceImpl() = default;

	virtual bool OnCreate(WorldRuntime& runtime) = 0;
	virtual bool OnStart(WorldRuntime& runtime) = 0;
	virtual void OnStop(WorldRuntime& runtime) = 0;
};

struct WorldInstanceIdentity
{
	WorldId id{ WorldId::Invalid() };
	WorldDefId defId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };
};

struct WorldInstanceCreateParams
{
	WorldInstanceIdentity identity;
	const WorldDef* def{ nullptr };
	WorldExecutionModel executionModel;
	const WorldTransferProfile* transferProfile{ nullptr };
	const IWorldTransferBinding* transferBinding{ nullptr };
	std::unique_ptr<IWorldInstanceImpl> impl;
};

class WorldInstance final
{
public:
	explicit WorldInstance(WorldInstanceCreateParams params);

	bool Initialize();
	void Shutdown();

	const WorldInstanceIdentity& GetIdentity() const { return _identity; }

	WorldId GetId() const { return _identity.id; }
	WorldDefId GetDefId() const { return _identity.defId; }
	uint64_t GetInstanceKey() const { return _identity.instanceKey; }

	const WorldDef* GetDef() const { return _def; }

	WorldRuntime& GetRuntime() { return _runtime; }
	const WorldRuntime& GetRuntime() const { return _runtime; }

	WorldExecutionModel& GetExecutionModel() { return _executionModel; }
	const WorldExecutionModel& GetExecutionModel() const { return _executionModel; }

	bool IsInitialized() const { return _initialized; }
	bool IsShutdown() const { return _shutdown; }
	bool IsFaulted() const { return _runtime.IsFaulted(); }

private:
	WorldInstanceIdentity _identity;
	const WorldDef* _def{ nullptr };

	WorldExecutionModel _executionModel;
	WorldRuntime _runtime;
	std::unique_ptr<IWorldInstanceImpl> _impl;

	bool _initialized{ false };
	bool _shutdown{ false };
};
