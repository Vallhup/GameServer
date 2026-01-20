#include "pch.h"
#include "Entity.h"
#include "Framework.h"

Entity EntityManager::Create()
{
    int id;

    if (not _freeIds.empty())
    {
        id = _freeIds.front();
        _freeIds.pop();
    }

    else
    {
        id = static_cast<int>(_generations.size());
        _generations.push_back(0);
    }

    return Entity{ id, _generations[id] };
}

Entity EntityManager::CreatePlayer(uint32 sessionId)
{
    Entity e = Create();

    Framework::Get().sessionToEntity[sessionId] = e;
    Framework::Get().entityToSession[e] = sessionId;

    return e;
}

void EntityManager::Destroy(Entity entity)
{
    const int id = entity.id;
    if (id >= _generations.size()) return;

    ++_generations[id];
    _freeIds.push(id);
}

void EntityManager::DestoryPlayer(Entity entity, uint32 sessionId)
{
    Destroy(entity);

    Framework::Get().entityToSession.erase(entity);
    Framework::Get().sessionToEntity.erase(sessionId);
}

bool EntityManager::IsAlive(Entity entity) const
{
    return entity.id < _generations.size() and
        _generations[entity.id] == entity.generation;
}
