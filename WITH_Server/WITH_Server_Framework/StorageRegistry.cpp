#include "pch.h"
#include "StorageRegistry.h"

void StorageRegistry::OnEntityDestroyed(Entity e)
{
	for (auto& storage : _storages)
	{
		if (storage)
			storage->OnEntityDestroyed(e);
	}
}

void StorageRegistry::Clear()
{
	for (auto& storage : _storages)
	{
		if (storage)
			storage->Clear();
	}
}

void StorageRegistry::EnsureNotFix() const
{
	if (_isFixed)
		throw std::logic_error("StorageRegistry is Fixed.");
}

void StorageRegistry::EnsureSlot(TypeId id)
{
	if (_storages.size() <= static_cast<size_t>(id))
		_storages.resize(static_cast<size_t>(id) + 1);
}
