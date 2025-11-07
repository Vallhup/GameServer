#pragma once

#include <concurrent_unordered_map.h>

class IObjectManager {
public:
	static int GenerateObjectId();

public:
	virtual ~IObjectManager() = default;

public:
	virtual void AddObject(std::unique_ptr<GameObject> object) = 0;
	virtual void RemoveObject(int objectId) = 0;

	virtual GameObject* GetGameObject(int objectId) const = 0;
	virtual std::vector<GameObject*> GetGameObjectList() const = 0;
};

class ObjectManager : public IObjectManager {
public:
	ObjectManager() = default;
	virtual ~ObjectManager() = default;

public:
	virtual void AddObject(std::unique_ptr<GameObject> object) override;
	virtual void RemoveObject(int objectId) override;

	virtual GameObject* GetGameObject(int objectId) const override;
	virtual std::vector<GameObject*> GetGameObjectList() const override;

private:
	concurrency::concurrent_unordered_map<int, std::unique_ptr<GameObject>> _objects;
};

