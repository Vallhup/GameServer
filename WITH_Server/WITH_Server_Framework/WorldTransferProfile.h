#pragma once

#include <memory>
#include <span>
#include <vector>

#include "IWorldTransferSerializer.h"
#include "WorldTransferTypes.h"

class WorldTransferProfile final {
public:
	WorldTransferProfile() = default;
	~WorldTransferProfile() = default;

	WorldTransferProfile(const WorldTransferProfile&) = delete;
	WorldTransferProfile& operator=(const WorldTransferProfile&) = delete;
	WorldTransferProfile(WorldTransferProfile&&) noexcept = default;
	WorldTransferProfile& operator=(WorldTransferProfile&&) noexcept = default;

public:
	bool Add(std::unique_ptr<IWorldTransferSerializer> serializer);
	bool Add(
		WorldTransferSerializerId serializerId,
		std::unique_ptr<IWorldTransferSerializer> serializer);
	bool Has(WorldTransferSerializerId serializerId) const noexcept;
	const IWorldTransferSerializer* Find(
		WorldTransferSerializerId serializerId) const noexcept;

	std::span<const IWorldTransferSerializer* const> Serializers() const noexcept;

	void Clear();

private:
	bool AddSerializerInternal(
		WorldTransferSerializerId serializerId,
		std::unique_ptr<IWorldTransferSerializer> serializer);

private:
	std::vector<std::unique_ptr<IWorldTransferSerializer>> _ownedSerializers;
	std::vector<IWorldTransferSerializer*> _orderedSerializers;
};
