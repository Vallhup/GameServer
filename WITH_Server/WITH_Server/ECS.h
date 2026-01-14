#pragma once

#include "Entity.h"
#include "SystemManager.h"
#include "ComponentStorage.h"

struct ECS {
	template<CompT T>
	ComponentStorage<T>& GetStorage()
	{
		static ComponentStorage<T> storage;
		return storage;
	}

	EntityManager entityMng;
	SystemManager systemMng;
};