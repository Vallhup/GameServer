#pragma once

#include <vector>
#include <memory>
#include <stdexcept>

#include "ComponentStorage.h"

inline TypeId NextTypeId()
{
	static std::atomic<TypeId> g_id{ 0 };
	return g_id.fetch_add(1);
}

template<CompT T>
TypeId TypeIdOf()
{
	static const TypeId id = NextTypeId();
	return id;
}

class StorageRegistry {
public:
	template<CompT T>
	ComponentStorage<T>& GetStorage()
	{
		const TypeId id = TypeIdOf<T>();
		if (_storages.size() <= id) 
			_storages.resize(id + 1);

		if (!_storages[id])
			_storages[id] = std::make_unique<ComponentStorage<T>>();

		return static_cast<ComponentStorage<T>&>(*_storages[id]);
	}

	template<CompT T>
	const ComponentStorage<T>& GetStorage() const
	{
		const TypeId id = TypeIdOf<T>();
		if (_storages.size() <= id)
			_storages.resize(id + 1);

		if (!_storages[id])
			_storages[id] = std::make_unique<ComponentStorage<T>>();

		return static_cast<const ComponentStorage<T>&>(*_storages[id]);
	}

	void OnEntityDestroyed(Entity e);
	void Clear();

private:
	// Storage 생성은 초기화 단계에서만 수행 -> Data Race 방지
	mutable std::vector<std::unique_ptr<IStorage>> _storages;
};

