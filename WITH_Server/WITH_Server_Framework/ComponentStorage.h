#pragma once

#include <vector>
#include <cassert>
#include <compare>
#include <iterator>
#include <type_traits>
#include <utility>

#include "Entity.h"
#include "Component.h"

using TypeId = int;

class ECSCore;
class WorldRuntime;
class WorldCommandBuffer;
class LifecycleCommandBuffer;

class IStorage
{
public:
	virtual ~IStorage() = default;

	virtual void OnEntityDestroyed(Entity e) = 0;
	virtual void Clear() = 0;

	virtual size_t Size() const = 0;
	virtual Entity EntityAt(size_t i) const = 0;
};

template<CompT T>
class ComponentStorage final : public IStorage {
	static constexpr int INVALID{ -1 };

public:
	template<bool IsConst>
	struct ItemRef
	{
		using entity_ref = std::conditional_t<IsConst, const Entity&, Entity&>;
		using comp_ref = std::conditional_t<IsConst, const T&, T&>;

		entity_ref entity;
		comp_ref component;

		operator std::pair<entity_ref, comp_ref>() const
		{
			return { entity, component };
		}
	};

	template<bool IsConst>
	struct ItemPtr
	{
		using entity_ptr = std::conditional_t<IsConst, const Entity*, Entity*>;
		using comp_ptr = std::conditional_t<IsConst, const T*, T*>;

		entity_ptr entity;
		comp_ptr component;
	};

	template<bool IsConst>
	class BasicIterator {
		using storage_t = std::conditional_t<IsConst, const ComponentStorage, ComponentStorage>;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = ItemRef<IsConst>;
		using reference = ItemRef<IsConst>;
		using pointer = ItemPtr<IsConst>*;

		BasicIterator() = default;
		BasicIterator(storage_t* s, size_t i)
			: _s(s), _i(i), _cache{}
		{
		}

		BasicIterator& operator++() { ++_i; return *this; }
		BasicIterator& operator--() { --_i; return *this; }

		BasicIterator operator++(int) { auto t = *this; ++(*this); return t; }
		BasicIterator operator--(int) { auto t = *this; --(*this); return t; }

		std::strong_ordering operator<=>(const BasicIterator& rhs) const
		{
			assert(_s == rhs._s);
			return _i <=> rhs._i;
		}

		bool operator==(const BasicIterator& rhs) const
		{
			return _s == rhs._s && _i == rhs._i;
		}

		BasicIterator& operator+=(difference_type n)
		{
			_i = static_cast<size_t>(static_cast<difference_type>(_i) + n);
			return *this;
		}

		BasicIterator& operator-=(difference_type n)
		{
			_i = static_cast<size_t>(static_cast<difference_type>(_i) - n);
			return *this;
		}

		BasicIterator operator+(difference_type n) const
		{
			return BasicIterator(_s, _i + n);
		}

		BasicIterator operator-(difference_type n) const
		{
			return BasicIterator(_s, _i - n);
		}

		difference_type operator-(const BasicIterator& rhs) const
		{
			assert(_s == rhs._s);
			return static_cast<difference_type>(_i) - static_cast<difference_type>(rhs._i);
		}

		reference operator*() const
		{
			assert(_s && _i < _s->Size());
			return reference{ _s->_entities[_i], _s->_dense[_i] };
		}

		pointer operator->() const
		{
			assert(_s && _i < _s->Size());
			_cache.entity = &_s->_entities[_i];
			_cache.component = &_s->_dense[_i];
			return &_cache;
		}

		reference operator[](difference_type n) const
		{
			return *(*this + n);
		}

	private:
		storage_t* _s{ nullptr };
		size_t _i{ 0 };
		mutable ItemPtr<IsConst> _cache;
	};

	using iterator = BasicIterator<false>;
	using const_iterator = BasicIterator<true>;

public:
	iterator begin() { return iterator(this, 0); }
	iterator end() { return iterator(this, Size()); }

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end() const { return const_iterator(this, Size()); }

	const_iterator cbegin() const { return const_iterator(this, 0); }
	const_iterator cend() const { return const_iterator(this, Size()); }

public:
	const Entity& EntityAtDense(size_t i) const
	{
		assert(i < _entities.size());
		return _entities[i];
	}

	int DenseIndex(Entity entity) const
	{
		if (entity.id < 0 || static_cast<size_t>(entity.id) >= _sparse.size())
			return INVALID;

		const int di = _sparse[entity.id];
		if (di == INVALID)
			return INVALID;

		if (_entities[di] != entity)
			return INVALID;

		return di;
	}

	T* GetComponent(Entity entity)
	{
		const int di = DenseIndex(entity);
		return (di == INVALID) ? nullptr : &_dense[di];
	}

	const T* GetComponent(Entity entity) const
	{
		const int di = DenseIndex(entity);
		return (di == INVALID) ? nullptr : &_dense[di];
	}

	bool HasComponent(Entity entity) const
	{
		return DenseIndex(entity) != INVALID;
	}

public:
	void OnEntityDestroyed(Entity e) override
	{
		if (HasComponent(e))
			RemoveComponentInternal(e);
	}

	void Clear() override
	{
		_dense.clear();
		_entities.clear();
		_sparse.clear();
	}

	size_t Size() const override
	{
		return _dense.size();
	}

	Entity EntityAt(size_t i) const override
	{
		assert(i < _entities.size());
		return _entities[i];
	}

private:
	template<typename... Args>
	T* AddComponentInternal(Entity entity, Args&&... args)
	{
		if (entity.IsNull())
			return nullptr;

		if (static_cast<size_t>(entity.id) >= _sparse.size())
			_sparse.resize(static_cast<size_t>(entity.id) + 1, INVALID);

		int& di = _sparse[entity.id];
		if (di != INVALID)
		{
			if (_entities[di] != entity)
			{
				assert(false && "AddComponentInternal called with stale Entity.");
				return nullptr;
			}

			assert(false && "AddComponentInternal called for existing component.");
			return nullptr;
		}

		di = static_cast<int>(_dense.size());
		_entities.push_back(entity);
		_dense.emplace_back(std::forward<Args>(args)...);
		return &_dense.back();
	}

	T* AddOrAssignComponentInternal(Entity entity, T value)
	{
		const int di = DenseIndex(entity);
		if (di == INVALID)
			return AddComponentInternal(entity, std::move(value));

		_dense[di] = std::move(value);
		return &_dense[di];
	}

	bool RemoveComponentInternal(Entity entity)
	{
		const int di = DenseIndex(entity);
		if (di == INVALID)
			return false;

		const int last = static_cast<int>(_dense.size() - 1);
		if (di != last)
		{
			_dense[di] = std::move(_dense[last]);
			_entities[di] = _entities[last];
			_sparse[_entities[di].id] = di;
		}

		_dense.pop_back();
		_entities.pop_back();
		_sparse[entity.id] = INVALID;
		return true;
	}

private:
	friend class ECSCore;
	friend class WorldRuntime;
	friend class WorldCommandBuffer;
	friend class LifecycleCommandBuffer;

	std::vector<T> _dense;
	std::vector<Entity> _entities;
	std::vector<int> _sparse;
};