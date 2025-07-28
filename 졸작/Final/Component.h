#pragma once

class GameObject;

class Component
{
public:
	virtual ~Component() = default;
	virtual void Init() {}
	virtual void Update() {}

	shared_ptr<GameObject> GetGameObject();

private:
	friend class GameObject;
	weak_ptr<GameObject> owner;
};
