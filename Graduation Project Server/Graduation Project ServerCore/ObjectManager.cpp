#include "pch.h"
#include "ObjectManager.h"

ObjectId IObjectManager::GenerateObjectId(ObjectType type)
{
    static std::atomic<int> _nextId{ 0 };
    return ObjectId{ _nextId++, type };
}

void ObjectManager::AddObject(const std::shared_ptr<GameObject>& object)
{
    std::unique_lock lock{ _mutex };
    _objects.insert(std::make_pair(object->GetId(), object));
}

void ObjectManager::RemoveObject(ObjectId objectId)
{
    std::unique_lock lock{ _mutex };
    _objects.erase(objectId);
}

std::shared_ptr<GameObject> ObjectManager::GetGameObject(ObjectId objectId) const
{
    std::shared_lock lock{ _mutex };

    auto it = _objects.find(objectId);
    if (it != _objects.end()) {
        return it->second;
    }

    return nullptr;
}
