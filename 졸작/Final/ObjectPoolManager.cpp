#include "pch.h"
#include "ObjectPoolManager.h"
#include "GameObject.h"

void ObjectPoolManager::Initialize(PoolType type, const PoolInfo& info)
{
	poolInfos[type] = info;
	
	// TODO : 기존 활성 Pool들 정리
	// --
	ClearPools(type);
	
	pools[type].reserve(info.maxSize);
	for (size_t i = 0; i < info.initSize; ++i) {
		auto obj = info.createFunc();
		obj->SetId(-1);
		pools[type].push_back(obj);
	}
}

void ObjectPoolManager::ClearPools(PoolType type)
{
	for (auto& pair : activeObjects[type])
		pair.second->SetId(-1);

	activeObjects[type].clear();
	usedIds[type].clear();
	pools[type].clear();
}
