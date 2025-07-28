#include "pch.h"
#include "GameObject.h"
#include "Component.h"

void GameObject::Update()
{
	for (auto& comp : components) {
		comp->Update();
	}
}