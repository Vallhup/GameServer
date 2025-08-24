#pragma once

class IObjectManager {
public:
	static int GenerateObjectId();

public:
	virtual ~IObjectManager() = default;

public:
	virtual void AddObject(const std::shared_ptr<GameObject>& object) = 0;
	virtual void RemoveObject(int objectId) = 0;

	virtual std::shared_ptr<GameObject> GetGameObject(int objectId) const = 0;

	virtual void Update(float deltaTime) = 0;
};

class ObjectManager : public IObjectManager {
public:
	ObjectManager() = default;
	virtual ~ObjectManager() = default;

public:
	virtual void AddObject(const std::shared_ptr<GameObject>& object) override;
	virtual void RemoveObject(int objectId) override;

	virtual std::shared_ptr<GameObject> GetGameObject(int objectId) const override;

	virtual void Update(float deltaTime) override;

private:
	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<GameObject>> _objects;
};