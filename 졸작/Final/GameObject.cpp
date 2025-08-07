#include "pch.h"
#include "GameObject.h"
#include "Component.h"

void GameObject::Update(float deltaTime)
{
	for (auto& comp : components) {
		comp->Update(deltaTime);
	}
}