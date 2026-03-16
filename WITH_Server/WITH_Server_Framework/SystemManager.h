#pragma once

#include <span>
#include <array>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include "System.h"

class ECS;
struct SystemMeta;

enum class SystemPhase : uint8_t { Pre, Graph, Post, Count };

template<typename T>
concept SysT = std::derived_from<T, System>;

struct SystemScheduleDesc
{
	System* system{ nullptr };
	const SystemMeta* meta{ nullptr };
	int registrationOrder{ -1 };
};

class SystemManager {
	static constexpr int kPhaseCnt{ static_cast<int>(SystemPhase::Count) };

public:
	SystemManager() { _rawSystemsDirty.fill(true); }

	template<SysT T, typename... Args>
	T* RegisterSystem(SystemPhase phase, Args&&... args)
	{
		static_assert(std::is_constructible_v<T, Args...>, 
			"System T is not constructible from Args...");

		const int index = static_cast<int>(phase);
		assert(index >= 0 && index < kPhaseCnt);

		auto sys = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = sys.get();

		const auto key = std::type_index(typeid(T));
		assert(_typeMap[index].find(key) == _typeMap[index].end());

		_systems[index].emplace_back(std::move(sys), _nextOrder++);
		_typeMap[index][key] = ptr;
		_rawSystemsDirty[index] = true;

		return ptr;
	}

	template<SysT T>
	T* GetSystem(SystemPhase phase)
	{
		const int index = static_cast<int>(phase);
		const auto key = std::type_index(typeid(T));

		auto it = _typeMap[index].find(key);
		if (it == _typeMap[index].end()) return nullptr;
		return static_cast<T*>(it->second);
	}

	std::span<System*> GetSystems(SystemPhase phase)
	{
		const int index = static_cast<int>(phase);
		RebuildRaw(index);
		return _rawSystems[index];
	}

	std::span<const System*> GetSystems(SystemPhase phase) const
	{
		const int index = static_cast<int>(phase);
		RebuildRaw(index);
		return _rawConstSystems[index];
	}

	std::vector<SystemScheduleDesc> BuildScheduleDescs(SystemPhase phase) const
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

private:
	void RebuildRaw(int index) const
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

	struct SystemEntry
	{
		std::unique_ptr<System> system;
		int registrationOrder{ -1 };
	};

	int _nextOrder{ 0 };

	std::array<std::vector<SystemEntry>, kPhaseCnt> _systems;
	std::array<std::unordered_map<std::type_index, System*>, kPhaseCnt> _typeMap;

	mutable std::array<bool, kPhaseCnt> _rawSystemsDirty;
	mutable std::array<std::vector<System*>, kPhaseCnt> _rawSystems;
	mutable std::array<std::vector<const System*>, kPhaseCnt> _rawConstSystems;
};