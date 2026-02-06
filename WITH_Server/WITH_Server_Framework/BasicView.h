#pragma once

#include <array>

#include "ECS.h"
#include "Entity.h"
#include "Component.h"
#include "ComponentStorage.h"

template<bool IsConst, typename GetTuple, typename ExTuple>
class BasicView;

template<bool IsConst, CompT... Get, CompT... Ex>
class BasicView<IsConst, std::tuple<Get...>, std::tuple<Ex...>> {
	static_assert(sizeof...(Get) > 0, "BasicView must have at least one component to get.");

	using ECS_ref = std::conditional_t<IsConst, const ECS&, ECS&>;

	template<CompT T>
	using comp_ref = std::conditional_t<IsConst, const T&, T&>;

public:
	explicit BasicView(ECS_ref ecs) : _ecs(ecs), _base(0)
	{
		((void)_ecs.GetStorage<Get>(), ...);
		if constexpr (sizeof...(Ex) > 0)
			((void)_ecs.GetStorage<Ex>(), ...);

		_pools = { static_cast<const IStorage*>(& _ecs.GetStorage<Get>())... };
		for (size_t i = 1; i < _pools.size(); ++i)
		{
			if (_pools[i]->Size() < _pools[_base]->Size())
				_base = i;
		}
	}

	class BasicViewIterator {
	public:
		BasicViewIterator(const BasicView* v, size_t i) : _view(v), _i(i) { Skip(); }

		BasicViewIterator& operator++() { ++_i; Skip(); return *this; }

		bool operator==(const BasicViewIterator& rhs) const { return _view == rhs._view && _i == rhs._i; }
		bool operator!=(const BasicViewIterator& rhs) const { return !(*this == rhs); }

		auto operator*() const
		{
			Entity e = _view->Base().EntityAt(_i);
			return _view->MakeTuple(e);
		}

	private:
		const BasicView* _view;
		size_t _i;

		void Skip()
		{
			const IStorage& base = _view->Base();
			const size_t n = base.Size();

			for(;_i < n; ++_i)
			{
				Entity e = base.EntityAt(_i);
				if (_view->IsValidEntity(e)) 
					break;
			}
		}
	};

	using iterator = BasicViewIterator;

	iterator begin() const { return iterator(this, 0); }
	iterator end()	 const { return iterator(this, Base().Size()); }

private:

	const IStorage& Base() const { return *_pools[_base]; }

	bool IsValidEntity(Entity e) const
	{
		if (!(_ecs.GetStorage<Get>().HasComponent(e) && ...))
			return false;

		if constexpr (sizeof...(Ex) > 0)
		{
			if (((_ecs.GetStorage<Ex>().HasComponent(e)) || ...))
				return false;
		}

		assert((_ecs.GetStorage<Get>().GetComponent(e) && ...));
		return true;
	}

	auto MakeTuple(Entity e) const
	{
		return std::tuple<Entity, comp_ref<Get>...>(e, *(_ecs.GetStorage<Get>().GetComponent(e))...);
	}


	ECS_ref _ecs;

	size_t _base;
	std::array<const IStorage*, sizeof...(Get)> _pools{};
};

// 1. foreach형태 지원? -> 고민 중
// 2. RuntimeView 지원? -> 아마 안 할듯