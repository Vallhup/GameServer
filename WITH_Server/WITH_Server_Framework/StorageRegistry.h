#pragma once

#include <atomic>
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
	void RegisterStorage()
	{
		EnsureNotFix();

		const TypeId id = TypeIdOf<T>();
		EnsureSlot(id);

		if (!_storages[id])
			_storages[id] = std::make_unique<ComponentStorage<T>>();
	}

	void Fix() noexcept { _isFixed = true; }
	bool IsFixed() const noexcept { return _isFixed; }

	template<CompT T>
	bool HasStorage() const noexcept
	{
		const TypeId id = TypeIdOf<T>();
		return
			id < static_cast<TypeId>(_storages.size()) &&
			_storages[id] != nullptr;
	}

	template<CompT T>
	ComponentStorage<T>& GetStorage()
	{
		if (auto* s = TryGetStorage<T>())
			return *s;

		throw std::logic_error("Storage is not registered.");
	}

	template<CompT T>
	const ComponentStorage<T>& GetStorage() const
	{
		if (auto* s = TryGetStorage<T>())
			return *s;

		throw std::logic_error("Storage is not registered.");
	}

	template<CompT T>
	ComponentStorage<T>* TryGetStorage() noexcept
	{
		return HasStorage<T>() ?
			static_cast<ComponentStorage<T>*>(_storages[TypeIdOf<T>()].get()) :
			nullptr;
	}

	template<CompT T>
	const ComponentStorage<T>* TryGetStorage() const noexcept
	{
		return HasStorage<T>() ?
			static_cast<const ComponentStorage<T>*>(_storages[TypeIdOf<T>()].get()) :
			nullptr;
	}

	void OnEntityDestroyed(Entity e);
	void Clear();

private:
	void EnsureNotFix() const;
	void EnsureSlot(TypeId id);


	// Storage 생성은 초기화 단계에서만 수행 -> Data Race 방지
	bool _isFixed{ false };
	std::vector<std::unique_ptr<IStorage>> _storages;
};

