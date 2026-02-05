#include "pch.h"
#include "SystemManager.h"

void SystemManager::Initialize(ECS& e)
{
}

const std::vector<System*> SystemManager::GetSystems() const
{
	std::vector<System*> out;
	for (const auto& system : _systems)
		out.push_back(system.get());

	return out;
}
