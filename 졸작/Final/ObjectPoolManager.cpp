#include "pch.h"
#include "ObjectPoolManager.h"
#include "GameObject.h"

void ObjectPoolManager::Initialize(PoolType type, const PoolInfo& info)
{
	poolInfos[type] = info;
	
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

shared_ptr<GameObject> ObjectPoolManager::GetGameObject(PoolType type)
{
	auto poolType = pools.find(type);
	if (poolType == pools.end()) {
		OutputDebugStringA("해당 타입의 풀이 초기화되지 않음\n");
		return nullptr;
	}

	for (auto& obj : poolType->second) {
		if (obj && obj->GetId() == -1) {
			return obj;
		}
	}

	OutputDebugStringA("사용 가능한 pool이 존재하지 않음\n");
	return nullptr;
}

shared_ptr<GameObject> ObjectPoolManager::FindActiveObject(PoolType type, int id)
{
	auto poolType = activeObjects.find(type);		// unordered_map<type, unordered_map<int, shared_ptr<GameObject>>> activeObjects;
	if (poolType == activeObjects.end()) {
		OutputDebugStringA("해당 타입의 풀이 만들어진 적이 없음\n");
		return nullptr;
	}

	auto& activeMap = poolType->second;			// unordered_map<int, shared_ptr<GameObject>> activeObjects;
	auto activeobj = activeMap.find(id);
	if (activeobj == activeMap.end()) {
		OutputDebugStringA("해당 타입의 활성화된 객체를 찾을 수 없음\n");
		return nullptr;
	}

	return activeobj->second;
}

bool ObjectPoolManager::ActivateObject(PoolType type, int id, shared_ptr<GameObject> obj)
{
	if (!obj || id <= 0) return false;

	if (usedIds[type].find(id) != usedIds[type].end()) {
		OutputDebugStringA("이미 사용되고 있는 ID입니다.\n");
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
		OutputDebugStringA("해당 타입의 풀이 만들어진 적이 없음\n");
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
