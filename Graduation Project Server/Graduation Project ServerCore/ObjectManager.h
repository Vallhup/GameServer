#pragma once

class IObjectManager {
public:
	static ObjectId GenerateObjectId(ObjectType type);

public:
	virtual ~IObjectManager() = default;

public:
	virtual void AddStaticObject(const std::shared_ptr<StaticGameObject>& object) = 0;
	virtual void AddDynamicObject(const std::shared_ptr<DynamicGameObject>& object) = 0;
	virtual void RemoveObject(ObjectId objectId) = 0;

	virtual std::shared_ptr<GameObject> GetGameObject(ObjectId objectId) const = 0;
	virtual std::shared_ptr<StaticGameObject> GetStaticObject(ObjectId objectId) const = 0;
	virtual std::shared_ptr<DynamicGameObject> GetDynamicObject(ObjectId objectId) const = 0;

	virtual void Update(float deltaTime) = 0;
};

class ObjectManager : public IObjectManager {
public:
	ObjectManager() = default;
	virtual ~ObjectManager() = default;

public:
	virtual void AddStaticObject(const std::shared_ptr<StaticGameObject>& object) override;
	virtual void AddDynamicObject(const std::shared_ptr<DynamicGameObject>& object) override;
	virtual void RemoveObject(ObjectId objectId) override;

	virtual std::shared_ptr<GameObject> GetGameObject(ObjectId objectId) const override;
	virtual std::shared_ptr<StaticGameObject> GetStaticObject(ObjectId objectId) const override;
	virtual std::shared_ptr<DynamicGameObject> GetDynamicObject(ObjectId objectId) const override;

	virtual void Update(float deltaTime) override;

private:
	mutable std::shared_mutex _staticMutex;
	std::unordered_map<ObjectId, std::shared_ptr<StaticGameObject>> _staticObjects;

	mutable std::shared_mutex _dynamicMutex;
	std::unordered_map<ObjectId, std::shared_ptr<DynamicGameObject>> _dynamicObjects;
};

