#pragma once

#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include "Component.h"
#include "DefaultWorldTransferSerializer.h"
#include "IWorldTransferSerializer.h"
#include "StorageRegistry.h"
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
	template<CompT T>
	bool AddDefault();

	template<CompT T>
	bool AddCustom(std::unique_ptr<IWorldTransferSerializer> serializer);

	bool Has(ComponentTypeId typeId) const noexcept;
	const IWorldTransferSerializer* Find(ComponentTypeId typeId) const noexcept;

	std::span<const IWorldTransferSerializer* const> Serializers() const noexcept;

	void Clear();

private:
	bool AddSerializerInternal(
		ComponentTypeId typeId,
		std::unique_ptr<IWorldTransferSerializer> serializer);

private:
	std::vector<std::unique_ptr<IWorldTransferSerializer>> _ownedSerializers;
	std::vector<IWorldTransferSerializer*> _orderedSerializers;
};

template<CompT T>
bool WorldTransferProfile::AddDefault()
{
	static_assert(
		std::is_trivially_copyable_v<T>,
		"Default world transfer serializer requires trivially copyable component type.");

	return AddSerializerInternal(
		TypeIdOf<T>(),
		std::make_unique<DefaultWorldTransferSerializer<T>>());
}

template<CompT T>
bool WorldTransferProfile::AddCustom(std::unique_ptr<IWorldTransferSerializer> serializer)
{
	if (!serializer)
		return false;

	const ComponentTypeId expectedTypeId = TypeIdOf<T>();
	if (serializer->GetComponentTypeId() != expectedTypeId)
		return false;

	return AddSerializerInternal(expectedTypeId, std::move(serializer));
}
