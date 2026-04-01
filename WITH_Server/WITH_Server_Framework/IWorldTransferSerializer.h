#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "ECSView.h"
#include "WorldTransferTypes.h"

class WorldRuntime;

class IWorldTransferSerializer
{
public:
	virtual ~IWorldTransferSerializer() = default;

	virtual ComponentTypeId GetComponentTypeId() const noexcept = 0;

	virtual bool Export(
		ECSView sourceView,
		Entity sourceEntity,
		std::vector<std::byte>& outBytes) const = 0;

	virtual bool Import(
		WorldRuntime& targetRuntime,
		Entity targetEntity,
		std::span<const std::byte> bytes) const = 0;
};
