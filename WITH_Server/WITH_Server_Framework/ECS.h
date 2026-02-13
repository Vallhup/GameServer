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

	template<SysT T, typename... Args>
	T* AddSystem(SystemPhase phase, Args&&... args)
	{
		return _systemMng.RegisterSystem<T>(phase, std::forward<Args>(args)...);
	}

private:
	
	template<bool IsConst, CompT... Get, CompT... Ex>
	static auto MakeView(std::conditional_t<IsConst, const ECS&, ECS&> ecs, Exclude<Ex...>)
	{
		return BasicView<IsConst, std::tuple<Get...>, std::tuple<Ex...>>(ecs);
	}

	friend class WorldRuntime;

	EntityManager _entityMng;
	SystemManager _systemMng;
	StorageRegistry _storageRegistry;
};

// 1. CommandBuffer
// 2. Dirty Tracking
// 3. Resource Registry
// 4. Access Metadata 구조화
// 5. Entity Identity 정책
// 6. Entity Mapping