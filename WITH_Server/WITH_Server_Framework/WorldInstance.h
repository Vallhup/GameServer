#pragma once

#include "WorldContentIds.h"

class IWorldInstanceImpl {
public:
	virtual ~IWorldInstanceImpl() = default;

	virtual void OnInit(WorldRuntime& rumtime) = 0;
	virtual void OnShutdown(WorldRuntime& rumtime) = 0;
	virtual void OnUpdate(WorldRuntime& rumtime, const double dT) = 0;
};

struct WorldDef;

struct WorldInstanceCreateParams
{
	WorldId id{ WorldId::Invalid() };
	WorldDefId defId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };

	const WorldDef* def{ nullptr };
};

class WorldInstance {
public:
	WorldInstance(
		const WorldInstanceCreateParams& params,
		std::unique_ptr<IWorldInstanceImpl> impl);

	void Init();
	void Shutdown();
	void Update(const double dT);

	WorldId GetId() const { return _id; }
	WorldDefId GetDefId() const { return _defId; }
	uint64_t GetInstanceKey() const { return _instanceKey; }

	WorldRuntime& GetRuntime() { return _runtime; }
	const WorldRuntime& GetRuntime() const { return _runtime; }

private:
	WorldId _id;
	WorldDefId _defId;
	uint64_t _instanceKey;

	bool _initialized{ false };
	bool _shutdown{ false };

	WorldRuntime _runtime;
	std::unique_ptr<IWorldInstanceImpl> _impl;
};

