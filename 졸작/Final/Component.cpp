#include "pch.h"
#include "Component.h"

shared_ptr<GameObject> Component::GetGameObject()
{
	return owner.lock();
}

