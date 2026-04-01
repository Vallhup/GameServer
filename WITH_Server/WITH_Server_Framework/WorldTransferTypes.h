#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ComponentStorage.h"
#include "Entity.h"

using ComponentTypeId = TypeId;

struct TransferComponentSnapshot
{
	ComponentTypeId typeId{};
	std::vector<std::byte> bytes;
};

struct TransferEntitySnapshot
{
	uint32_t sessionId{ 0 };
	Entity sourceEntity{ Entity::Null() };
	std::vector<TransferComponentSnapshot> components;
};
