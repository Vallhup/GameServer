#include "pch.h"
#include "WorldTransferProfile.h"

bool WorldTransferProfile::Add(std::unique_ptr<IWorldTransferSerializer> serializer)
{
	if (!serializer)
		return false;

	const IWorldTransferSerializer* const raw = serializer.get();
	const WorldTransferSerializerId serializerId = raw->GetSerializerId();
	return AddSerializerInternal(serializerId, std::move(serializer));
}

bool WorldTransferProfile::Add(
	WorldTransferSerializerId serializerId,
	std::unique_ptr<IWorldTransferSerializer> serializer)
{
	if (!serializer)
		return false;

	return AddSerializerInternal(serializerId, std::move(serializer));
}

bool WorldTransferProfile::Has(
	WorldTransferSerializerId serializerId) const noexcept
{
	return Find(serializerId) != nullptr;
}

const IWorldTransferSerializer* WorldTransferProfile::Find(
	WorldTransferSerializerId serializerId) const noexcept
{
	for (const IWorldTransferSerializer* serializer : _orderedSerializers)
	{
		if (serializer != nullptr &&
			serializer->GetSerializerId() == serializerId)
		{
			return serializer;
		}
	}

	return nullptr;
}

std::span<const IWorldTransferSerializer* const> WorldTransferProfile::Serializers() const noexcept
{
	return std::span<const IWorldTransferSerializer* const>(
		_orderedSerializers.data(),
		_orderedSerializers.size());
}

void WorldTransferProfile::Clear()
{
	_orderedSerializers.clear();
	_ownedSerializers.clear();
}

bool WorldTransferProfile::AddSerializerInternal(
	WorldTransferSerializerId serializerId,
	std::unique_ptr<IWorldTransferSerializer> serializer)
{
	if (!serializer)
		return false;

	if (serializerId == InvalidWorldTransferSerializerId)
		return false;

	if (serializer->GetSerializerId() != serializerId)
		return false;

	if (Has(serializerId))
		return false;

	IWorldTransferSerializer* raw = serializer.get();
	_ownedSerializers.push_back(std::move(serializer));
	_orderedSerializers.push_back(raw);
	return true;
}
