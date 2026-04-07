#pragma once

#include <cstring>
#include <span>
#include <type_traits>
#include <vector>

#include "IWorldTransferSerializer.h"
#include "StorageRegistry.h"
#include "WorldRuntime.h"

template<CompT T>
class DefaultWorldTransferSerializer final : public IWorldTransferSerializer {
	static_assert(
		std::is_trivially_copyable_v<T>,
		"DefaultWorldTransferSerializer supports only trivially copyable component types.");

public:
	ComponentTypeId GetComponentTypeId() const noexcept override
	{
		return TypeIdOf<T>();
	}

	bool Export(
		ECSView sourceView,
		Entity sourceEntity,
		std::vector<std::byte>& outBytes) const override
	{
		const T* component = sourceView.GetComponent<T>(sourceEntity);
		if (component == nullptr)
			return false;

		outBytes.resize(sizeof(T));
		std::memcpy(outBytes.data(), component, sizeof(T));
		return true;
	}

	bool Import(
		WorldRuntime& targetRuntime,
		Entity targetEntity,
		std::span<const std::byte> bytes) const override
	{
		if (bytes.size() != sizeof(T))
			return false;

		T value{};
		std::memcpy(&value, bytes.data(), sizeof(T));
		targetRuntime.DeferredUpsertComponent<T>(targetEntity, std::move(value));
		return !targetRuntime.IsFaulted();
	}
};
