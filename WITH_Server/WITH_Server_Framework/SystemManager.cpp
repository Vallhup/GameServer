#include "pch.h"
#include "SystemManager.h"

std::span<System*> SystemManager::GetSystems()
{
	RebuildRaw();
	return _rawSystems;
}

std::span<const System*> SystemManager::GetSystems() const
{
	RebuildRaw();
	return _rawConstSystems;
}

std::vector<SystemScheduleDesc> SystemManager::BuildScheduleDescs() const
{
	std::vector<SystemScheduleDesc> out;
	out.reserve(_systems.size());

	for (const SystemEntry& entry : _systems)
	{
		System* sys = entry.system.get();
		out.emplace_back(sys, &sys->Meta(), entry.registrationOrder);
	}

	std::sort(out.begin(), out.end(),
		[](const auto& a, const auto& b)
		{
			return a.registrationOrder < b.registrationOrder;
		});

	return out;
}

void SystemManager::Clear()
{
	
	_systems.clear();
	_typeMap.clear();
	_rawSystems.clear();
	_rawConstSystems.clear();
	_rawSystemsDirty = true;
	_nextOrder = 0;
}

void SystemManager::RebuildRaw() const
{
	if (!_rawSystemsDirty) return;

	_rawSystems.clear();
	_rawConstSystems.clear();

	_rawSystems.reserve(_systems.size());
	_rawConstSystems.reserve(_systems.size());

	for (const SystemEntry& entry : _systems)
	{
		System* sys = entry.system.get();
		_rawSystems.push_back(sys);
		_rawConstSystems.push_back(sys);
	}

	_rawSystemsDirty = false;
}