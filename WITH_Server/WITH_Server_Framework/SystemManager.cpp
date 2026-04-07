#include "pch.h"
#include "SystemManager.h"

std::span<System*> SystemManager::GetSystems(SystemPhase phase)
{
	const int index = static_cast<int>(phase);
	RebuildRaw(index);
	return _rawSystems[index];
}

std::span<const System*> SystemManager::GetSystems(SystemPhase phase) const
{
	const int index = static_cast<int>(phase);
	RebuildRaw(index);
	return _rawConstSystems[index];
}

std::vector<SystemScheduleDesc> SystemManager::BuildScheduleDescs(SystemPhase phase) const
{
	const int index = static_cast<int>(phase);

	std::vector<SystemScheduleDesc> out;
	out.reserve(_systems[index].size());

	for (const SystemEntry& entry : _systems[index])
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
	for (int i = 0; i < kPhaseCnt; ++i)
	{
		_systems[i].clear();
		_typeMap[i].clear();
		_rawSystems[i].clear();
		_rawConstSystems[i].clear();
		_rawSystemsDirty[i] = true;
	}

	_nextOrder = 0;
}

void SystemManager::RebuildRaw(int index) const
{
	if (!_rawSystemsDirty[index]) return;

	_rawSystems[index].clear();
	_rawConstSystems[index].clear();

	_rawSystems[index].reserve(_systems[index].size());
	_rawConstSystems[index].reserve(_systems[index].size());

	for (const SystemEntry& entry : _systems[index])
	{
		System* sys = entry.system.get();
		_rawSystems[index].push_back(sys);
		_rawConstSystems[index].push_back(sys);
	}

	_rawSystemsDirty[index] = false;
}