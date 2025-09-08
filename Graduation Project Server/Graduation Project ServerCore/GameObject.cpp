#include "pch.h"
#include "GameObject.h"

GameObject::~GameObject()
{
	_components.clear();
	_types.clear();
}

void GameObject::LogicUpdate(float deltaTime)
{
	for (auto& comp : _components) {
		comp->LogicUpdate(deltaTime);
	}
}

void GameObject::NetworkUpdate()
{
	for (auto& comp : _components) {
		comp->NetworkUpdate();
	}
}