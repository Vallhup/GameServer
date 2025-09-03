#include "pch.h"
#include "GameObject.h"

GameObject::~GameObject()
{
	_components.clear();
	_types.clear();
}

void GameObject::Update(float deltaTime)
{
	for (auto& comp : _components) {
		comp->Update(deltaTime);
	}
}