#include "pch.h"
#include "CollisionComponent.h"

void CollisionComponent::Update(float deltaTime)
{
	for (const auto& shape : _shapes) {
		//shape->CheckCollision()
	}
}

void CollisionComponent::Activate(CollisionType type)
{
	for (auto& shape : _shapes) {
		if (shape->GetCollisionType() == type) {
			shape->SetActive(true);
		}
	}
}

void CollisionComponent::Deactivate(CollisionType type)
{
	for (auto& shape : _shapes) {
		if (shape->GetCollisionType() == type) {
			shape->SetActive(false);
		}
	}
}

void CollisionComponent::ActivateAll()
{
	for (auto& shape : _shapes) {
		shape->SetActive(true);
	}
}

void CollisionComponent::DeactivateAll()
{
	for (auto& shape : _shapes) {
		shape->SetActive(false);
	}
}
