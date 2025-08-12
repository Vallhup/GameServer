#include "pch.h"
#include "ComponentHost.h"

void ComponentHost::Update(float deltaTime)
{
    for (auto& comp : _components) {
        comp->Update(deltaTime);
    }
}