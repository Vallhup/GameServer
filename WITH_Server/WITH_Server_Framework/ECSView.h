#pragma once

#include <span>
#include <tuple>

#include "ECSCore.h"

class ECSView {
public:
	explicit ECSView(ECSCore& core)
		: _core(&core)
	{
	}

	explicit ECSView(const ECSCore& core)
		: _core(const_cast<ECSCore*>(&core))
	{
	}

	bool IsAlive(Entity e) const
	{
		return _core->IsAlive(e);
	}

	std::span<const Entity> AliveEntities() const
	{
		return _core->AliveEntities();
	}

	template<CompT T>
	bool HasStorage() const
	{
		return _core->HasStorage<T>();
	}

	template<CompT T>
	const ComponentStorage<T>* TryGetStorage() const noexcept
	{
		return _core->TryGetStorage<T>();
	}

	template<CompT T>
	const T* GetComponent(Entity e) const
	{
		return _core->GetComponent<T>(e);
	}

	template<CompT T>
	bool HasComponent(Entity e) const
	{
		return _core->HasComponent<T>(e);
	}

	template<CompT... Get>
	auto View() const
	{
		return _core->View<Get...>();
	}

	template<CompT... Get, CompT... Ex>
	auto View(Exclude<Ex...>) const
	{
		return _core->View<Get...>(Exclude<Ex...>{});
	}

private:
	ECSCore* _core{ nullptr };
};