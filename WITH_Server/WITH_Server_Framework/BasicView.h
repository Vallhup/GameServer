#pragma once

#include <array>

#include "ECS.h"
#include "Entity.h"
#include "Component.h"
#include "ComponentStorage.h"
#include "ViewFwd.h"

// NOTE:
//  1. Iteration order is not stable.
//  2. Required storage missing -> empty view.
//  3. Missing exclude storage is treated as empty exclude set.
//  4. View creation never creates storages.
//  5. References returned by iteration are valid only until structural mutation.
//  6. Structural mutation during iteration is forbidden.

template<bool IsConst, CompT... Get, CompT... Ex>
class BasicView<IsConst, std::tuple<Get...>, std::tuple<Ex...>> {
	static_assert(sizeof...(Get) > 0, "BasicView must have at least one component to get.");

	using ECS_ref = std::conditional_t<IsConst, const ECS&, ECS&>;

	template<CompT T>
	using storage_ptr = std::conditional_t<IsConst, const ComponentStorage<T>*, ComponentStorage<T>*>;

	template<CompT T>
	using comp_ptr = std::conditional_t<IsConst, const T*, T*>;

	template<CompT T>
	using comp_ref = std::conditional_t<IsConst, const T&, T&>;

public:
	explicit BasicView(ECS_ref ecs) { Init(ecs); }

	class BasicViewIterator {
	public:
		BasicViewIterator(const BasicView* v, size_t i) : _view(v), _i(i) { Skip(); }

		BasicViewIterator& operator++() { ++_i; Skip(); return *this; }

		bool operator==(const BasicViewIterator& rhs) const { return _view == rhs._view && _i == rhs._i; }
		bool operator!=(const BasicViewIterator& rhs) const { return !(*this == rhs); }

		auto operator*() const { return MakeTuple(_entity); }

	private:
		void Skip()
		{
			if (_view == nullptr || _view->_empty)
				return;

			const IStorage& base = _view->Base();
			const size_t n = base.Size();

			for (; _i < n; ++_i)
			{
				_entity = base.EntityAt(_i);

				_ptrs = std::apply(
					[&](auto*... s)
					{
						return std::tuple<comp_ptr<Get>...>{ s->GetComponent(_entity)... };
					}, _view->_getStorages);

				const bool getCheck = std::apply(
					[&](auto*... p)
					{
						return ((p != nullptr) && ...);
					}, _ptrs);

				if (!getCheck) continue;

				if constexpr (sizeof...(Ex) > 0)
				{
					const bool exCheck = std::apply(
						[&](auto*... s)
						{
							return (((s != nullptr) && (s->GetComponent(_entity) != nullptr)) || ...);
						}, _view->_exStorages);

					if (exCheck) continue;
				}

				break;
			}
		}

		auto MakeTuple(Entity e) const
		{
			return std::apply(
				[&](auto*... p)
				{
					return std::tuple<Entity, comp_ref<Get>...>(e, (*p)...);
				}, _ptrs);
		}

		const BasicView* _view;
		size_t _i;
		Entity _entity;
		std::tuple<comp_ptr<Get>...> _ptrs;
	};

	using iterator = BasicViewIterator;

	iterator begin() const { return iterator(this, 0); }
	iterator end()	 const { return iterator(this, Size()); }

	bool Empty() const noexcept { return _empty; }
	size_t Size() const noexcept { return _empty ? 0 : Base().Size(); }

private:
	void Init(ECS_ref ecs)
	{
		_getStorages = std::tuple{ ecs.TryGetStorage<Get>()... };

		const bool getCheck = std::apply(
			[](auto*... s)
			{
				return ((s != nullptr) && ...);
			}, _getStorages);

		if (!getCheck)
		{
			_empty = true;
			return;
		}

		if constexpr (sizeof...(Ex) > 0)
			_exStorages = std::tuple{ ecs.TryGetStorage<Ex>()... };

		FillPools(std::index_sequence_for<Get...>{});
		
		_base = 0;
		for (size_t i = 1; i < _pools.size(); ++i)
		{
			if (_pools[i]->Size() < _pools[_base]->Size())
				_base = i;
		}
	}

	template<size_t... I>
	void FillPools(std::index_sequence<I...>)
	{
		_pools = { static_cast<const IStorage*>(std::get<I>(_getStorages))... };
	}

	const IStorage& Base() const { return *_pools[_base]; }

	bool _empty{ false };
	size_t _base;

	std::array<const IStorage*, sizeof...(Get)> _pools{};
	std::tuple<storage_ptr<Get>...> _getStorages;
	std::tuple<storage_ptr<Ex>...> _exStorages;
};