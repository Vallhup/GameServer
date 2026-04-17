#pragma once

#include <cstdint>
#include <functional>

#include "WorldContentIds.h"

struct WorldResolveKey
{
	WorldDefId defId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };

	constexpr auto operator<=>(const WorldResolveKey&) const = default;
	constexpr bool operator==(const WorldResolveKey&) const = default;
};

namespace std
{
	template<>
	struct hash<WorldResolveKey>
	{
		size_t operator()(const WorldResolveKey& key) const noexcept
		{
			size_t h1 = std::hash<uint64_t>()(static_cast<uint64_t>(key.defId));
			size_t h2 = std::hash<uint64_t>()(key.instanceKey);

			// boost::hash_combine 방식
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
		}
	};
}