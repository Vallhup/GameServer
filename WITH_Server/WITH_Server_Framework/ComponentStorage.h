#pragma once

#include <vector>

#include "Entity.h"
#include "Component.h"

template<CompT T>
class ComponentStorage {
	static constexpr int INVALID{ -1 };

public:
	T* AddComponent(Entity entity)
	{
		if (entity.id >= _sparse.size())
			_sparse.resize(entity.id + 1, INVALID);

		if (_sparse[entity.id] != INVALID)
			return &_dense[_sparse[entity.id]];

		int di = _dense.size();
		_sparse[entity.id] = di;
		_entities.push_back(entity);
		_dense.emplace_back();

		return &_dense.back();
	}

	T* GetComponent(Entity entity)
	{
		if (entity.id >= _sparse.size()) return nullptr;
		return _sparse[entity.id] == INVALID ?
			nullptr : &_dense[_sparse[entity.id]];
	}

	const T* GetComponent(Entity entity) const
	{
		if (entity.id >= _sparse.size()) return nullptr;
		return _sparse[entity.id] == INVALID ?
			nullptr : &_dense[_sparse[entity.id]];
	}

	void RemoveComponent(Entity entity)
	{
		if (entity.id >= _sparse.size() or _dense.empty()) return;
		if (_sparse[entity.id] == INVALID) return;

		int di = _sparse[entity.id];
		int last = static_cast<int>(_dense.size()) - 1;

		std::swap(_dense[di], _dense[last]);
		std::swap(_entities[di], _entities[last]);
		_sparse[_entities[di].id] = di;

		_dense.pop_back();
		_entities.pop_back();

		_sparse[entity.id] = INVALID;
	}

	bool HasComponent(Entity entity) const
	{
		return GetComponent(entity) != nullptr;
	}

	const std::vector<T>& Dense()    const { return _dense; }
	std::vector<T>& Dense() { return _dense; }
	const std::vector<Entity>& Entities() const { return _entities; }
	std::vector<Entity>& Entities() { return _entities; }

	int DenseIndex(Entity entity) const
	{
		return (entity.id < _sparse.size()) ? _sparse[entity.id] : INVALID;
	}

	int Size() const { return static_cast<int>(_dense.size()); }

private:
	std::vector<T> _dense;
	std::vector<Entity> _entities;

	std::vector<int> _sparse;
};