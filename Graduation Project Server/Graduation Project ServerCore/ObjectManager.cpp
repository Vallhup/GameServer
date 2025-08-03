#include "pch.h"
#include "ObjectManager.h"

ObjectId IObjectManager::GenerateObjectId(ObjectType type)
{
    static std::atomic<int> _nextId{ 0 };
    return ObjectId{ _nextId++, type };
}

void ObjectManager::AddStaticObject(const std::shared_ptr<StaticGameObject>& object)
{
    std::unique_lock lock{ _staticMutex };
    _staticObjects.insert(std::make_pair(IObjectManager::GenerateObjectId(Static), object));
}

void ObjectManager::AddDynamicObject(const std::shared_ptr<DynamicGameObject>& object)
{
    std::unique_lock lock{ _dynamicMutex };
    _dynamicObjects.insert(std::make_pair(IObjectManager::GenerateObjectId(Dynamic), object));
}

void ObjectManager::RemoveObject(ObjectId objectId)
{
    if (objectId.type == Static) {
        std::unique_lock lock{ _staticMutex };
        _staticObjects.erase(objectId);
    }

    else if (objectId.type == Dynamic) {
        std::unique_lock lock{ _dynamicMutex };
        _dynamicObjects.erase(objectId);
    }
}

std::shared_ptr<GameObject> ObjectManager::GetGameObject(ObjectId objectId) const
{
    if (objectId.type == Static) {
        std::shared_lock lock{ _staticMutex };

        auto it = _staticObjects.find(objectId);
        if (it != _staticObjects.end()) {
            return it->second;
        }

        return nullptr;
    }

    else if (objectId.type == Dynamic) {
        std::shared_lock lock{ _dynamicMutex };

        auto it = _dynamicObjects.find(objectId);
        if (it != _dynamicObjects.end()) {
            return it->second;
        }

        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<StaticGameObject> ObjectManager::GetStaticObject(ObjectId objectId) const
{
    if (objectId.type == Static) {
        std::shared_lock lock{ _staticMutex };

        auto it = _staticObjects.find(objectId);
        if (it != _staticObjects.end()) {
            return it->second;
        }

        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<DynamicGameObject> ObjectManager::GetDynamicObject(ObjectId objectId) const
{
    if (objectId.type == Dynamic) {
        std::shared_lock lock{ _dynamicMutex };

        auto it = _dynamicObjects.find(objectId);
        if (it != _dynamicObjects.end()) {
            return it->second;
        }

        return nullptr;
    }

    return nullptr;
}

void ObjectManager::Update(float deltaTime)
{
    std::vector<std::shared_ptr<DynamicGameObject>> objects;
    {
        std::shared_lock lock{ _dynamicMutex };
        for (const auto& [id, object] : _dynamicObjects) {
            objects.push_back(object);
        }
    }

    for (const auto& object : objects) {
        if (object) {
            object->Update(deltaTime);
        }
    }
}
