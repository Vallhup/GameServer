#pragma once

#include <span>
#include <tuple>
#include <utility>

#include "EntityManager.h"
#include "ComponentStorage.h"
#include "StorageRegistry.h"
#include "BasicView.h"
#include "ViewFwd.h"

class WorldRuntime;

class ECSCore {
public:
	bool IsAlive(Entity e) const
	{
		return _entityMng.IsAlive(e);
	}

	std::span<const Entity> AliveEntities() const
	{
		return _entityMng.AliveEntities();
	}

public:
	template<CompT T>
	void RegisterStorage()
	{
		_storageRegistry.RegisterStorage<T>();
	}

	void FixStorages()
	{
		_storageRegistry.Fix();
	}

	bool StoragesFixed() const noexcept
	{
		return _storageRegistry.IsFixed();
	}

	void Clear()
	{
		_storageRegistry.Clear();
		_entityMng.Clear();
	}

public:
	template<CompT T>
	bool HasStorage() const
	{
		return _storageRegistry.HasStorage<T>();
	}

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
	ComponentStorage<T>* TryGetStorage() noexcept
	{
		return _storageRegistry.TryGetStorage<T>();
	}

	template<CompT T>
	const ComponentStorage<T>* TryGetStorage() const noexcept
	{
		return _storageRegistry.TryGetStorage<T>();
	}

public:
	template<CompT T>
	T* GetComponent(Entity e)
	{
		auto* storage = TryGetStorage<T>();
		return storage ? storage->GetComponent(e) : nullptr;
	}

	template<CompT T>
	const T* GetComponent(Entity e) const
	{
		auto* storage = TryGetStorage<T>();
		return storage ? storage->GetComponent(e) : nullptr;
	}

	template<CompT T>
	bool HasComponent(Entity e) const
	{
		auto* storage = TryGetStorage<T>();
		return storage ? storage->HasComponent(e) : false;
	}

public:
	template<CompT... Get>
	auto View()
	{
		return BasicView<false, std::tuple<Get...>, std::tuple<>>(*this);
	}

	template<CompT... Get, CompT... Ex>
	auto View(Exclude<Ex...>)
	{
		return BasicView<false, std::tuple<Get...>, std::tuple<Ex...>>(*this);
	}

	template<CompT... Get>
	auto View() const
	{
		return BasicView<true, std::tuple<Get...>, std::tuple<>>(*this);
	}

	template<CompT... Get, CompT... Ex>
	auto View(Exclude<Ex...>) const
	{
		return BasicView<true, std::tuple<Get...>, std::tuple<Ex...>>(*this);
	}

private:
	bool MaterializeReservedEntityImmediate(Entity reserved)
	{
		return _entityMng.MaterializeReserved(reserved);
	}

	bool DestroyEntityImmediate(Entity e)
	{
		if (!_entityMng.Destroy(e))
			return false;

		_storageRegistry.OnEntityDestroyed(e);
		return true;
	}

	template<CompT T, typename... Args>
	T* AddComponentImmediate(Entity e, Args&&... args)
	{
		return GetStorage<T>().AddComponentInternal(e, std::forward<Args>(args)...);
	}

	template<CompT T>
	T* AddOrAssignComponentImmediate(Entity e, T value)
	{
		return GetStorage<T>().AddOrAssignComponentInternal(e, std::move(value));
	}

	template<CompT T>
	bool RemoveComponentImmediate(Entity e)
	{
		auto* storage = TryGetStorage<T>();
		if (!storage)
			return false;

		return storage->RemoveComponentInternal(e);
	}

private:
	friend class WorldRuntime;

	EntityManager _entityMng;
	StorageRegistry _storageRegistry;
};
