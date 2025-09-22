#pragma once

class GameObject;

enum class PoolType{
	KNIGHT,
	TANKER,
	NINJA,
	DRAGON,
	CASTLE,
	// 추가될거임
};

struct PoolInfo {
	size_t initSize = 1;
	size_t maxSize = 5;
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
	unordered_map<PoolType, vector<shared_ptr<GameObject>>> pools;						// 타입 별 pool
	unordered_map<PoolType, unordered_map<int, shared_ptr<GameObject>>> activeObjects;	// 타입 별 활성 Object들
	unordered_map<PoolType, PoolInfo> poolInfos;										// 풀 설정 정보
	unordered_map<PoolType, set<int>> usedIds;											// 현재 사용중인 ID들
};

