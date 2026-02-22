#pragma once

#include "WorldId.h"
#include "WorldDesc.h"

class WorldRegistry;
class WorldScheduler;

class WorldService {
public:
	explicit WorldService(WorldRegistry& reg)
		: _reg(reg), _scheduler(nullptr) {}

	bool InitSquare();

	WorldId ResolveTargetWorld(WorldType type, uint64 key);

	void OnPlayerEnter(WorldId worldId, uint32 count = 1);
	void OnPlayerLeave(WorldId worldId, uint32 count = 1);

	void CommitDestroy();

	void SetScheduler(WorldScheduler& scheduler) { _scheduler = &scheduler; }
	void Clear();

private:
	struct Instance
	{
		WorldType type{ WorldType::None };
		uint64 instanceKey{ 0 };
		int32 playerCount{ 0 };
		bool pendingDestroy{ true };
	};

	struct InstKey
	{
		WorldType type{ WorldType::None };
		uint64 instanceKey{ 0 };

		bool operator==(const InstKey& other) const noexcept
		{
			return type == other.type && instanceKey == other.instanceKey;
		}
	};

	struct InstKeyHash
	{
		size_t operator()(const InstKey& key) const noexcept
		{
			size_t h1 = std::hash<uint8>()(static_cast<uint8>(key.type));
			size_t h2 = std::hash<uint64>()(key.instanceKey);

			// boost::hash_combine ¹æ½Ä
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
		}
	};

	WorldId CreateWorld(WorldType type, uint64 key);

	WorldRegistry& _reg;
	WorldScheduler* _scheduler;
	WorldId _squareWorldId;

	std::unordered_map<InstKey, WorldId, InstKeyHash> _resolved;
	std::unordered_map<WorldId, Instance> _instances;
};

