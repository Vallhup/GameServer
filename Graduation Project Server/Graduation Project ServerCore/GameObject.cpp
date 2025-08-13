#include "pch.h"
#include "GameObject.h"

GameObject::~GameObject()
{
	for (auto& uniqeCmp : _components) {
		uniqeCmp->Deregister();
	}
	
	_components.clear();
	_types.clear();
}
