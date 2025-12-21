#include "GameWorld.h"

Instance* GameWorld::CreateInstance()
{
    int id = _nextId++;
    auto instance = std::make_unique<Instance>(id);
    auto* ptr = instance.get();
    _instances.try_emplace(id, std::move(instance));
    return ptr;
}

Instance* GameWorld::GetInstance(int id)
{
    auto it = _instances.find(id);
    return (it != _instances.end()) ? it->second.get() : nullptr;
}
