#include "pch.h"
#include "ECS.h"

bool ECS::IsAlive(Entity e) const
{
	return _entityMng.IsAlive(e);
}

Entity ECS::CreateEntityImmediate()
{
	return _entityMng.Create();
}

void ECS::DestroyEntityImmediate(Entity e)
{
	_entityMng.Destroy(e);
	_storageRegistry.OnEntityDestroyed(e);
}
