#include "pch.h"
#include "Component.h"

GameObject* Component::GetGameObject()
{
	return owner;
}

