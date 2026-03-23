#pragma once

#include "Entity.h"
#include "Component.h"
#include "System.h"

enum class BuildStage : uint8_t
{
	Register,
	SpawnInitial,
	Finalized
};

struct WorldDesc;

#include "WorldRuntime.h"

class WorldBuilder final {
public:
	WorldBuilder(WorldRuntime& rt, const WorldDesc& desc)
		: _rt(rt), _desc(desc) { }

 	template<CompT... Ts>
	WorldBuilder& Components()
	{
		RequireStage(BuildStage::Register);
		(_rt.GetECS().template RegisterStorage<Ts>(), ...);
		return *this;
	}

	template<SysT T, typename... Args>
	WorldBuilder& PreSystem(Args&&... args)
	{
		RequireStage(BuildStage::Register);
		_rt.AddSystem<T>(SystemPhase::Pre, _rt, std::forward<Args>(args)...);
		return *this;
	}

	template<SysT T, typename... Args>
	WorldBuilder& GraphSystem(Args&&... args)
	{
		RequireStage(BuildStage::Register);
		_rt.AddSystem<T>(SystemPhase::Graph, _rt, std::forward<Args>(args)...);
		return *this;
	}

	template<SysT T, typename... Args>
	WorldBuilder& PostSystem(Args&&... args)
	{
		RequireStage(BuildStage::Register);
		_rt.AddSystem<T>(SystemPhase::Post, _rt, std::forward<Args>(args)...);
		return *this;
	}

	Entity SpawnEmpty();
	
	template<CompT... T>
	Entity Spawn(T&&... comps)
	{
		Entity entity = SpawnEmpty();
		(_rt.GetECS().AddComponentImmediate<std::decay_t<T>>(entity, std::forward<T>(comps)), ...);
		return entity;
	}

	void BeginInitialSpawns();
	void Commit();

	WorldRuntime& Runtime() { return _rt; }
	const WorldRuntime& Runtime() const { return _rt; }

private:
	void RequireStage(BuildStage expected) const;

	WorldRuntime& _rt;
	const WorldDesc& _desc;
	BuildStage _stage{ BuildStage::Register };
};

