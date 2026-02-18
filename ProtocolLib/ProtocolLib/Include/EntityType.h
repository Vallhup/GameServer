#pragma once

#include "types.h"

enum class EntityType : uint8 {
	None,

	Knight,
	Lancer,

	First_Boss,
	Mid_Boss,
	Final_Boss,

	Count
};

inline uint32 ToInt(EntityType type) { return static_cast<uint32>(type); }

namespace std {
	template<>
	struct hash<EntityType> {
		size_t operator()(const EntityType& type) const noexcept
		{
			return std::hash<uint32>()(ToInt(type));
		}
	};
}