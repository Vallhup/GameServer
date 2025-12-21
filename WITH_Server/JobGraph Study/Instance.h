#pragma once

#include "Entity.h"

class Instance {
public:
	Instance(int id) : _id(id) {}

	int GetId() const { return _id; }

	void AddEntity(Entity e) { _entities.push_back(e); }
	const std::vector<Entity>& GetEntities() const { return _entities; }

private:
	int _id;
	std::vector<Entity> _entities;
};