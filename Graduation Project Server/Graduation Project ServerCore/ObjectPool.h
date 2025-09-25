#pragma once

// ObjectPool 사용하려고 예상하는 객체
// 
// 1. SendOver
// 2. ODBC Handle
// 3. Thread Pool도 이 ObjectPool로 합칠까?

// Placement new
//  - 이미 확보된 메모리 공간에 객체를 생성하는 것
//  - 메모리 할당과 객체 생성 과정을 분리할 수 있음
//  - new (메모리 포인터) 객체타입(생성자 Arguments) 형태의 문법
//  - delete로 지우면 메모리 해제까지 해버리니까 쓰면 안됨
//  - 소멸자만 따로 호출해주기

template<typename T, size_t Size>
class ObjectPool {
	struct ObjectSlot {
		// Placement new로 객체를 생성하기 위해
		// 미리 메모리 할당이 필요함
		alignas(T) std::byte data[sizeof(T)];
	};

public:
	ObjectPool() = default;
	~ObjectPool();

public:
	// 객체마다 생성자에 다른 인자가 들어가야하는 경우를 위해
	// 생성자에서 일괄 초기화 방식 -> 외부에서 얻을 때 필요한 인자를 주는 방식으로 변경
	// 스마트포인터 쓰는 거처럼 쓸 수 있지 않을까?
	template<typename... Args>
	T* Acquire(Args&&... args);

	void Release(T* obj);

private:
	size_t GetFreeObjectIndex();

private:
	// 객체를 그대로 들고 있어야하나? 아니면 Pointer로?
	std::array<ObjectSlot, Size> _objects;
	std::bitset<Size> _inUse;
};

template<typename T, size_t Size>
template<typename ...Args>
inline T* ObjectPool<T, Size>::Acquire(Args&& ...args)
{
	size_t idx = GetFreeObjectIndex();
	if (idx == Size) return nullptr;

	_inUse.set(idx, true);
	return new (&_objects[idx].data) T(std::forward<Args>(args)...);
}

template<typename T, size_t Size>
inline ObjectPool<T, Size>::~ObjectPool()
{
	for (size_t i = 0; i < Size; ++i) {
		if (_inUse.test(i)) {
			reinterpret_cast<T*>(_objects[i].data)->~T();
		}
	}
}

template<typename T, size_t Size>
inline void ObjectPool<T, Size>::Release(T* obj)
{
	size_t idx = reinterpret_cast<ObjectSlot*>(obj) - &_objects[0];
	obj->~T();
	_inUse.reset(idx);
}

template<typename T, size_t Size>
inline size_t ObjectPool<T, Size>::GetFreeObjectIndex()
{
	for (size_t i = 0; i < Size; ++i) {
		if (not _inUse.test(i)) {
			return i;
		}
	}

	return Size;
}
