#include "pch.h"
#include "ECS.h"

Entity ECS::CreateEntity()
{
	return _entityMng.Create();
}

void ECS::DestroyEntity(Entity e)
{
	_entityMng.Destroy(e);
	_storageRegistry.OnEntityDestroyed(e);
}

bool ECS::IsAlive(Entity e) const
{
	return _entityMng.IsAlive(e);
}