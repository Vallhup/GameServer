#pragma once

#include <span>
#include <array>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <vector>
#include <cassert>

#include "System.h"

class ECS;

struct SystemScheduleDesc
{
	System* system{ nullptr };
	const ExecMeta* meta{ nullptr };
	int registrationOrder{ -1 };
};

class SystemManager {
public:
	SystemManager() { _rawSystemsDirty = true; }

	template<SysT T, typename... Args>
	T* RegisterSystem(Args&&... args)
	{
		static_assert(std::is_constructible_v<T, Args...>, 
			"System T is not constructible from Args...");

		auto sys = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = sys.get();

		const auto key = std::type_index(typeid(T));
		assert(_typeMap.find(key) == _typeMap.end());

		_systems.emplace_back(std::move(sys), _nextOrder++);
		_typeMap[key] = ptr;
		_rawSystemsDirty = true;

		return ptr;
	}

	template<SysT T>
	T* GetSystem()
	{
		const auto key = std::type_index(typeid(T));

		auto it = _typeMap.find(key);
		if (it == _typeMap.end()) return nullptr;
		return static_cast<T*>(it->second);
	}

	template<SysT T>
	const T* GetSystem() const
	{
		const auto key = std::type_index(typeid(T));

		auto it = _typeMap.find(key);
		if (it == _typeMap.end()) return nullptr;
		return static_cast<const T*>(it->second);
	}

	std::span<System*> GetSystems();
	std::span<const System*> GetSystems() const;

	std::vector<SystemScheduleDesc> BuildScheduleDescs() const;

	void Clear();

private:
	void RebuildRaw() const;

	struct SystemEntry
	{
		std::unique_ptr<System> system;
		int registrationOrder{ -1 };
	};

private:
	int _nextOrder{ 0 };

	std::vector<SystemEntry> _systems;
	std::unordered_map<std::type_index, System*> _typeMap;

	mutable bool _rawSystemsDirty;
	mutable std::vector<System*> _rawSystems;
	mutable std::vector<const System*> _rawConstSystems;
};