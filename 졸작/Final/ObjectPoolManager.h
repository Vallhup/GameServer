#pragma once

class GameObject;

enum class PoolType{
	KNIGHT,
	TANKER,
	NINJA,
	DRAGON,
	CASTLE,
};

struct PoolInfo {
	size_t poolSize = 1;
	function<shared_ptr<GameObject>()> createFunc;
};

class ObjectPoolManager
{
public:
	void Initialize(PoolType type, const PoolInfo& info);
	void ClearPools(PoolType type);

	shared_ptr<GameObject> GetGameObject(PoolType type);
	shared_ptr<GameObject> FindActiveObject(PoolType type, int id);

	bool ActivateObject(PoolType type, int id, shared_ptr<GameObject> obj);
	bool DeactivateObject(PoolType type, int id);

private:
	unordered_map<PoolType, vector<shared_ptr<GameObject>>> pools;						
	unordered_map<PoolType, unordered_map<int, shared_ptr<GameObject>>> activeObjects;	
	unordered_map<PoolType, PoolInfo> poolInfos;										
	unordered_map<PoolType, set<int>> usedIds;											
};

