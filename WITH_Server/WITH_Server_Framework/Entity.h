#pragma once

#include <xhash>

struct Entity
{
	int id;
	int generation;

	Entity(int i = InvalidId, int g = 0)
		: id(i), generation(g) {}

	static constexpr int InvalidId{ std::numeric_limits<int>::min() };
	static Entity Null() { return Entity{}; }
	constexpr bool IsNull() const { return id == InvalidId; }

	bool operator==(const Entity& other) const noexcept
	{
		return (id == other.id) && (generation == other.generation);
	}

	bool operator<(const Entity& other) const noexcept
	{
		if (id != other.id)
			return id < other.id;

		else
			return generation < other.generation;
	}
};

namespace std 
{
	template<>
	struct hash<Entity> 
	{
		size_t operator()(const Entity& e) const noexcept
		{
			size_t h1 = std::hash<int>()(e.id);
			size_t h2 = std::hash<int>()(e.generation);

			// boost::hash_combine ¹æ½Ä
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
		}
	};
}