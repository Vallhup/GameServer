#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "ECSView.h"
#include "WorldTransferTypes.h"

class WorldRuntime;

struct WorldTransferExportContext
{
	ECSView sourceView;
	uint32_t sessionId{ 0 };
	Entity sourceEntity{ Entity::Null() };
	NetId netId{ NetId::Invalid() };
};

struct WorldTransferImportContext
{
	WorldRuntime& targetRuntime;
	uint32_t sessionId{ 0 };
	Entity sourceEntity{ Entity::Null() };
	Entity targetEntity{ Entity::Null() };
	NetId netId{ NetId::Invalid() };
};

class IWorldTransferSerializer
{
public:
	virtual ~IWorldTransferSerializer() = default;

	virtual WorldTransferSerializerId GetSerializerId() const noexcept = 0;

	virtual bool Export(
		const WorldTransferExportContext& context,
		std::vector<std::byte>& outBytes) const = 0;

	virtual bool Import(
		const WorldTransferImportContext& context,
		std::span<const std::byte> bytes) const = 0;
};
