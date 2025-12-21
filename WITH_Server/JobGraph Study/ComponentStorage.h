#pragma once

#include <unordered_map>
#include "Entity.h"

struct Component;

template<typename T>
concept CompT = std::is_base_of_v<Component, T>;

template<CompT T>
class ComponentStorage {
	static constexpr int INVALID{ -1 };

	struct Iterator {
		size_t i;
		ComponentStorage<T>* storage;

		Iterator(size_t i, ComponentStorage<T>* s) : i(i), storage(s) {}
		Iterator& operator++() { ++i; return *this; }
		bool operator!=(const Iterator& other) const { return i != other.i; }

		auto operator*() const
		{
			return std::pair<Entity, T&>(
				storage->_entities[i],
				storage->_dense[i]
			);
		}
	};

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

	const std::vector<T>&      Dense()    const { return _dense; }
	      std::vector<T>&	   Dense()          { return _dense; }
	const std::vector<Entity>& Entities() const { return _entities; }
		  std::vector<Entity>& Entities()	    { return _entities; }

	int DenseIndex(Entity entity) const
	{
		return (entity.id < _sparse.size()) ? _sparse[entity.id] : INVALID;
	}

	Iterator begin() { return { 0, this }; }
	Iterator end() { return { _dense.size(), this}; }

private:
	std::vector<T> _dense;
	std::vector<Entity> _entities;

	std::vector<int> _sparse;
};

