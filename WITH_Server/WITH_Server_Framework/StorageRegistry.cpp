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