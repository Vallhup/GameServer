#pragma once

#include "SystemManager.h"
#include "StorageRegistry.h"

class ECS {
public:
	Entity CreateEntity();
	void DestroyEntity(Entity e);
	bool IsAlive(Entity e) const;

	template<CompT T>
	ComponentStorage<T>& GetStorage()
	{
		return _storageRegistry.GetStorage<T>();
	}

	template<CompT T>
	const ComponentStorage<T>& GetStorage() const
	{
		return _storageRegistry.GetStorage<T>();
	}

	template<CompT T>
	void EnsureStorage()
	{
		_storageRegistry.GetStorage<T>();
	}

private:
	EntityManager _entityMng;
	SystemManager _systemMng;
	StorageRegistry _storageRegistry;
};