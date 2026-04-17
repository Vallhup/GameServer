#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Entity.h"
#include "NetId.h"

using WorldTransferSerializerId = uint32_t;
constexpr WorldTransferSerializerId InvalidWorldTransferSerializerId = 0;

struct TransferPayloadSnapshot
{
	WorldTransferSerializerId serializerId{ InvalidWorldTransferSerializerId };
	std::vector<std::byte> bytes;
};

struct TransferEntitySnapshot
{
	uint32_t sessionId{ 0 };
	Entity sourceEntity{ Entity::Null() };
	NetId netId{ NetId::Invalid() };
	std::vector<TransferPayloadSnapshot> payloads;
};

struct ImportedTransferEntity
{
	uint32_t sessionId{ 0 };
	Entity sourceEntity{ Entity::Null() };
	Entity targetEntity{ Entity::Null() };
	NetId netId{ NetId::Invalid() };
};
