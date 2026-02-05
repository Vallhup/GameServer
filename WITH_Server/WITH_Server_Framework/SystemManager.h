#pragma once

#include <memory>
#include "System.h"

class ECS;

template<typename T>
concept SysT = std::derived_from<T, System>;

class SystemManager {
public:
	template<SysT T, typename... Args>
	T* RegisterSystem(Args&&... args)
	{
		auto sys = std::make_unique<T>(std::forward<Args>(args)...);
		sys->stableOrder = _nextOrder++;

		T* ptr = sys.get();
		_systems.push_back(std::move(sys));
		return ptr;
	}

	void Initialize(ECS& e);

	template<SysT T>
	T* GetSystem()
	{
		for (const auto& sys : _systems)
		{
			if (auto* casted = dynamic_cast<T*>(sys.get()))
				return casted;
		}

		return nullptr;
	}

	const std::vector<System*> GetSystems() const;

private:
	int _nextOrder{ 0 };
	std::vector<std::unique_ptr<System>> _systems;
};