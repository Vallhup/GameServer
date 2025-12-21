#pragma once

#include <memory>
#include <vector>

#include "System.h"
#include "JobGraph.h"

class ThreadPool;
struct ECS;

template<typename T>
concept SysT = std::is_base_of_v<System, T>;

class SystemManager {
public:
	template<SysT T, typename... Args>
	T* RegisterSystem(Args&&... args)
	{
		auto sys = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = sys.get();
		_systems.push_back(std::move(sys));
		return ptr;
	}

	void Initalize(ECS& ecs, JobGraph& graph);

	template<SysT T>
	System* GetSystem();

	const std::vector<System*> GetSystems() const;

private:
	std::vector<std::unique_ptr<System>> _systems;
};

template<SysT T>
inline System* SystemManager::GetSystem()
{
	for (const auto& sys : _systems)
	{
		if (auto* casted = dynamic_cast<T*>(sys.get()))
			return casted;
	}

	return nullptr;
}
