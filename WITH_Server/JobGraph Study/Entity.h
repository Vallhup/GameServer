#pragma once

#include <vector>
#include <queue>

struct Entity {
	int id;
	int generation;

	Entity(int id = -1, int g = 0) 
		: id(id), generation(g) {}

	bool operator==(const Entity& other) const noexcept
	{
		return (id == other.id) and (generation == other.generation);
	}
};

namespace std {
	template<>
	struct hash<Entity> {
		size_t operator()(const Entity& e) const noexcept
		{
			size_t h1 = std::hash<int>()(e.id);
			size_t h2 = std::hash<int>()(e.generation);
			
			// boost::hash_combine ¹æ½Ä
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
		}
	};
}

class EntityManager {
public:
	Entity Create();
	Entity CreatePlayer(int sessionId);

	void Destroy(Entity entity);
	bool IsAlive(Entity entity) const;

private:
	std::vector<int> _generations;
	std::queue<int> _freeIds;
};

