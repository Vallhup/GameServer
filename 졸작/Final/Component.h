#pragma once

class GameObject;

class Component
{
public:
	virtual ~Component() = default;
	virtual void Init() {}
	virtual void Update(float deltaTime) {}

	GameObject* GetGameObject();

private:
	friend class GameObject;
	GameObject* owner = nullptr;
};
