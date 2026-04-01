#include "pch.h"
#include "WorldTransferProfile.h"

bool WorldTransferProfile::Has(ComponentTypeId typeId) const noexcept
{
	return Find(typeId) != nullptr;
}

const IWorldTransferSerializer* WorldTransferProfile::Find(ComponentTypeId typeId) const noexcept
{
	for (const IWorldTransferSerializer* serializer : _orderedSerializers)
	{
		if (serializer != nullptr && serializer->GetComponentTypeId() == typeId)
			return serializer;
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
	ComponentTypeId typeId,
	std::unique_ptr<IWorldTransferSerializer> serializer)
{
	if (!serializer)
		return false;

	if (serializer->GetComponentTypeId() != typeId)
		return false;

	if (Has(typeId))
		return false;

	IWorldTransferSerializer* raw = serializer.get();
	_ownedSerializers.push_back(std::move(serializer));
	_orderedSerializers.push_back(raw);
	return true;
}
