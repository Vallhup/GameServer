#pragma once

#include <stack>
#include <array>

#include "SendBuffer.h"
#include "LF_Stack.h"

// 개선점
//
// 1. Free Index 구조에서 Object 자체를 Node로 하는 Stack 구조로 변환
// 2. LF_Stack의 성능 개선
//
// 3. (추가) 동적 배열 Pooling을 위해 Memory Pool 구현

template<typename T, size_t Size>
class ObjectPool {
	struct ObjectSlot {
		alignas(T) std::byte data[sizeof(T)];
	};

public:
	ObjectPool();
	~ObjectPool() = default;

public:
	template<typename... Args>
	T* Acquire(Args&&... args);
	void Release(T* obj);

private:
	bool IsFromPool(T* obj);

private:
	std::array<ObjectSlot, Size> _objects;
	LF_Stack _freeIdxs;
};

template<typename T, size_t Size>
inline ObjectPool<T, Size>::ObjectPool()
{
	for (size_t i = 0; i < Size; ++i)
		_freeIdxs.push(i);
}

template<typename T, size_t Size>
template<typename ...Args>
inline T* ObjectPool<T, Size>::Acquire(Args&& ...args)
{
	if (_freeIdxs.empty())
		return new T(std::forward<Args>(args)...);

	size_t idx = _freeIdxs.pop();
	return new (&_objects[idx].data) T(std::forward<Args>(args)...);
}

template<typename T, size_t Size>
inline void ObjectPool<T, Size>::Release(T* obj)
{
	if (obj == nullptr) return;
	if (IsFromPool(obj)) 
	{
		obj->~T();
		size_t idx = (reinterpret_cast<std::byte*>(obj) - _objects[0].data) / sizeof(T);
		_freeIdxs.push(idx);
	}

	else
		delete obj;
}

template<typename T, size_t Size>
inline bool ObjectPool<T, Size>::IsFromPool(T* obj)
{
	auto* ptr = reinterpret_cast<std::byte*>(obj);
	auto* begin = reinterpret_cast<std::byte*>(&_objects[0]);
	auto* end = reinterpret_cast<std::byte*>(&_objects[Size - 1]);

	return (ptr >= begin and ptr <= end);
}