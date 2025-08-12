#pragma once

class IObjectManager {
public:
	static ObjectId GenerateObjectId(ObjectType type);

public:
	virtual ~IObjectManager() = default;

public:
	virtual void AddObject(const std::shared_ptr<GameObject>& object) = 0;
	virtual void RemoveObject(ObjectId objectId) = 0;

	virtual std::shared_ptr<GameObject> GetGameObject(ObjectId objectId) const = 0;

	virtual void Update(float deltaTime) = 0;
};

class ObjectManager : public IObjectManager {
public:
	ObjectManager() = default;
	virtual ~ObjectManager() = default;

public:
	virtual void AddObject(const std::shared_ptr<GameObject>& object) override;
	virtual void RemoveObject(ObjectId objectId) override;

	virtual std::shared_ptr<GameObject> GetGameObject(ObjectId objectId) const override;

	virtual void Update(float deltaTime) override;

private:
	mutable std::shared_mutex _mutex;
	std::unordered_map<ObjectId, std::shared_ptr<GameObject>> _objects;
};

