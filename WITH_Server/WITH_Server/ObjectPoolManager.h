#pragma once

#include "ObjectPool.hpp"

class SendBuffer;

class ObjectPoolManager {
public:
	static ObjectPoolManager& Get()
	{
		static ObjectPoolManager instance;
		return instance;
	}

	// TODO : ObjectPool Ãß°¡
	ObjectPool<SendBuffer, 8192> sendBuffers;

private:
	ObjectPoolManager() = default;
	~ObjectPoolManager() = default;

	ObjectPoolManager(const ObjectPoolManager&) = delete;
	ObjectPoolManager& operator=(const ObjectPoolManager&) = delete;
};