#pragma once

#include "EntityManager.h"
#include "ComponentStorage.h"
#include "StorageRegistry.h"
#include "BasicView.h"
#include "ViewFwd.h"

class ECS {
public:
	bool IsAlive(Entity e) const;

	std::span<const Entity> AliveEntities() const { return _entityMng.AliveEntities(); }

	template<CompT T>
	void RegisterStorage()
	{
		_storageRegistry.RegisterStorage<T>();
	}

	void FixStorages()
	{
		_storageRegistry.Fix();
	}

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
		return BasicView<false, std::tuple<Get...>, std::tuple<Ex...>>(*this);
	}

	// TEMP
	Entity CreateEntityImmediate();

	template<CompT T, typename... Args>
	T* AddComponentImmediate(Entity e, Args&&... args)
	{
		auto& s = _storageRegistry.GetStorage<T>();
		return s.EmplaceComponent(e, std::forward<Args>(args)...);
	}

private:
	

	template<CompT T>
	void RemoveComponentImmediate(Entity e)
	{
		if (auto* s = _storageRegistry.TryGetStorage<T>())
			s->RemoveComponent(e);
	}

	
	void DestroyEntityImmediate(Entity e);

	friend class WorldRuntime;
	friend class CommandBuffer;

	EntityManager _entityMng;
	StorageRegistry _storageRegistry;
};


// TODO LIST
// 
// 1. CommandBuffer (완료)
// 2. Dirty Tracking
// 3. Resource Registry
// 4. Access Metadata 구조화 (완료)
// 5. Entity Identity 정책
// 6. Entity Mapping


// JoinView
// 1. foreach형태 지원? -> 고민 중
// 2. RuntimeView 지원? -> 아마 안 할듯