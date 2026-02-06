#pragma once

#include "EntityManager.h"
#include "SystemManager.h"
#include "StorageRegistry.h"
#include "BasicView.h"

template<CompT... Ex>
struct Exclude {};

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
		((void)_storageRegistry.GetStorage<T>());
	}

	template<CompT... Get>
	auto View()
	{
		return BasicView<false, std::tuple<Get...>, std::tuple<>>(*this);
	}

	template<CompT... Get, CompT... Ex>
	auto View(Exclude<Ex...> ex)
	{
		return MakeView<false, Get...>(*this, ex);
	}

	template<CompT... Get>
	auto View() const
	{
		return BasicView<true, std::tuple<Get...>, std::tuple<>>(*this);
	}

	template<CompT... Get, CompT... Ex>
	auto View(Exclude<Ex...> ex) const
	{
		return MakeView<true, Get...>(*this, ex);
	}

private:
	
	template<bool IsConst, CompT... Get, CompT... Ex>
	static auto MakeView(std::conditional_t<IsConst, const ECS&, ECS&> ecs, Exclude<Ex...>)
	{
		return BasicView<IsConst, std::tuple<Get...>, std::tuple<Ex...>>(ecs);
	}


	EntityManager _entityMng;
	SystemManager _systemMng;
	StorageRegistry _storageRegistry;
};

// 1. JoinView
// 2. CommandBuffer
// 3. Dirty Tracking
// 4. Resource Registry
// 5. Access Metadata 구조화
// 6. Entity Identity 정책