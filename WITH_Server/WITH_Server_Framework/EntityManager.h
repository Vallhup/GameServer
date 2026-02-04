#pragma once

struct Entity;

class EntityManager {
public:
	Entity Create();

	bool Destroy(Entity e);

	bool IsAlive(Entity e) const;

private:
	std::vector<int> _generations;
	std::vector<int> _freeIds;

	size_t _aliveCount{ 0 };
};

