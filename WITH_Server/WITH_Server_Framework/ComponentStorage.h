#pragma once

#include <vector>
#include <cassert>

#include "Entity.h"
#include "Component.h"

template<CompT T>
class ComponentStorage {
	static constexpr int INVALID{ -1 };

public:
	template<bool IsConst>
	struct ItemRef {
		using entity_ref = std::conditional_t<IsConst, const Entity&, Entity&>;
		using comp_ref	 = std::conditional_t<IsConst, const T&, T&>;

		entity_ref entity;
		comp_ref   component;

		operator std::pair<entity_ref, comp_ref>() const
		{
			return { entity, component };
		}
	};

	template<bool IsConst>
	struct ItemPtr {
		using entity_ptr = std::conditional_t<IsConst, const Entity*, Entity*>;
		using comp_ptr	 = std::conditional_t<IsConst, const T*, T*>;

		entity_ptr entity;
		comp_ptr   component;
	};

	template<bool IsConst>
	class BasicIterator {
		using storage_t = std::conditional_t<IsConst, const ComponentStorage, ComponentStorage>;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type	= std::ptrdiff_t;
		using value_type		= ItemRef<IsConst>;
		using reference			= ItemRef<IsConst>;
		using pointer			= ItemPtr<IsConst>*;

		// 생성자
		BasicIterator() = default;
		BasicIterator(storage_t* s, size_t i) : _s(s), _i(i) {}

		// 증감연산자
		BasicIterator& operator++() { ++_i; return *this; }
		BasicIterator& operator--() { --_i; return *this; }

		BasicIterator operator++(int) { auto t = *this; ++(*this); return t; }
		BasicIterator operator--(int) { auto t = *this; --(*this); return t; }

		// 관계연산자
		std::strong_ordering operator<=>(const BasicIterator& rhs) const { assert(_s == rhs._s); return _i <=> rhs._i; }
		bool operator==(const BasicIterator& rhs) const { return _s == rhs._s && _i == rhs._i; }
		bool operator!=(const BasicIterator& rhs) const { return !(*this == rhs); }

		// 산술연산자
		BasicIterator& operator+=(difference_type n) { _i = size_t(difference_type(_i) + n); return*this; }
		BasicIterator& operator-=(difference_type n) { _i = size_t(difference_type(_i) - n); return*this; }

		BasicIterator operator+(difference_type n) const { return BasicIterator(_s, _i + n); }
		BasicIterator operator-(difference_type n) const { return BasicIterator(_s, _i - n); }

		difference_type operator-(const BasicIterator& rhs) const 
		{
			assert(_s == rhs._s);
			return difference_type(_i) - difference_type(rhs._i); 
		}

		// 접근연산자
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
		storage_t* _s;
		size_t     _i;
		mutable ItemPtr<IsConst> _cache;
	};

	using iterator = BasicIterator<false>;
	using const_iterator = BasicIterator<true>;

	size_t Size() const { return _dense.size(); }

	iterator begin() { return iterator(this, 0); }
	iterator end() { return iterator(this, Size()); }

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end() const { return const_iterator(this, Size()); }

	const_iterator cbegin() const { return const_iterator(this, 0); }
	const_iterator cend() const { return const_iterator(this, Size()); }


public:
	T* AddComponent(Entity entity)
	{
		if (entity.id >= _sparse.size())
			_sparse.resize(entity.id + 1, INVALID);

		int& di = _sparse[entity.id];
		if (di != INVALID)
		{
			if (_entities[di] != entity)
			{
				assert(false && "AddComponent clled with stale Entity.");
				return nullptr;
			}

			return &_dense[di];
		}

		di = static_cast<int>(_dense.size());
		_entities.push_back(entity);
		_dense.emplace_back();

		return &_dense.back();
	}

	T* GetComponent(Entity entity)
	{
		if (entity.id >= _sparse.size()) return nullptr;

		int di = _sparse[entity.id];
		const bool invalidCheck =
			(di == INVALIE) ||				 // valid check
			(_entities[di] != entity);		 // generation check

		if (invalidCheck) return nullptr;
		else return &_dense[di];
	}

	const T* GetComponent(Entity entity) const
	{
		if (entity.id >= _sparse.size()) return nullptr;

		int di = _sparse[entity.id];
		const bool invalidCheck =
			(di == INVALIE) ||				 // valid check
			(_entities[di] != entity);		 // generation check

		if (invalidCheck) return nullptr;
		else return &_dense[di];
	}

	void RemoveComponent(Entity entity)
	{
		if (entity.id >= _sparse.size() || _dense.empty()) return;
		if (_sparse[entity.id] == INVALID) return;

		int di = _sparse[entity.id];
		int last = static_cast<int>(_dense.size()) - 1;

		if (di != last)
		{
			std::swap(_dense[di], _dense[last]);
			std::swap(_entities[di], _entities[last]);
			_sparse[_entities[di].id] = di;
		}

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
		if (entity.id >= _sparse.size()) return INVALID;

		const int di = _sparse[entity.id];
		const bool invalidCheck =
			(di == INVALID) ||
			(_entities[di] != entity);

		if (invalidCheck) return INVALID;
		else return di;
	}

private:
	std::vector<T> _dense;
	std::vector<Entity> _entities;
	std::vector<int> _sparse;
};