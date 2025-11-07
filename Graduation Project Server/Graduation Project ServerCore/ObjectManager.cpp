#include "pch.h"
#include "ObjectManager.h"

int IObjectManager::GenerateObjectId()
{
    static std::atomic<int> _nextId{ 1 };
    return _nextId++;
}

void ObjectManager::AddObject(std::unique_ptr<GameObject> object)
{
    _objects.insert(std::make_pair(object->GetId(), std::move(object)));
}

void ObjectManager::RemoveObject(int objectId)
{
    //_objects.erase(objectId);
}

GameObject* ObjectManager::GetGameObject(int objectId) const
{
    auto it = _objects.find(objectId);
    if (it != _objects.end()) {
        return it->second.get();
    }

    return nullptr;
}

std::vector<GameObject*> ObjectManager::GetGameObjectList() const
{
    std::vector<GameObject*> objectList;
    objectList.reserve(_objects.size());

    for (const auto& [id, object] : _objects) {
        objectList.push_back(object.get());
    }

    return objectList;
}