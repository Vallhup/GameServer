#pragma once

#include <unordered_map>
#include <memory>

#include "Instance.h"

class GameWorld {
public:
	Instance* CreateInstance();
	Instance* GetInstance(int id);

	const auto& GetInstances() const { return _instances; }

private:
	int _nextId{ 0 };
	std::unordered_map<int, std::unique_ptr<Instance>> _instances;
};

