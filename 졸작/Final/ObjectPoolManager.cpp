#include "pch.h"
#include "ObjectPoolManager.h"
#include "GameObject.h"

void ObjectPoolManager::Initialize(PoolType type, const PoolInfo& info)
{
	poolInfos[type] = info;
	
	ClearPools(type);
	
	pools[type].reserve(info.poolSize);
	for (size_t i = 0; i < info.poolSize; ++i) {
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

shared_ptr<GameObject> ObjectPoolManager::GetGameObject(PoolType type)
{
	auto poolType = pools.find(type);
	if (poolType == pools.end()) {
		return nullptr;
	}

	for (auto& obj : poolType->second) {
		if (obj && obj->GetId() == -1) {
			return obj;
		}
	}

	return nullptr;
}

shared_ptr<GameObject> ObjectPoolManager::FindActiveObject(PoolType type, int id)
{
	auto poolType = activeObjects.find(type);
	if (poolType == activeObjects.end()) {
		return nullptr;
	}

	auto& activeMap = poolType->second;
	auto activeobj = activeMap.find(id);
	if (activeobj == activeMap.end()) {
		return nullptr;
	}

	return activeobj->second;
}

bool ObjectPoolManager::ActivateObject(PoolType type, int id, shared_ptr<GameObject> obj)
{
	if (!obj || id <= 0) return false;

	if (usedIds[type].find(id) != usedIds[type].end()) {
		return false;
	}

	obj->SetId(id);
	activeObjects[type][id] = obj;
	usedIds[type].insert(id);

	return true;
}

bool ObjectPoolManager::DeactivateObject(PoolType type, int id)
{
	if (id <= 0) return false;

	auto poolIt = activeObjects.find(type);
	if (poolIt == activeObjects.end()) {
		return false;  
	}

	auto& activeObjs = poolIt->second;
	auto activeObj = activeObjs.find(id);

	if (activeObj != activeObjs.end()) {
		activeObj->second->SetId(-1);
		activeObjs.erase(activeObj);
		usedIds[type].erase(id);
		return true;
	}

	return false;
}
