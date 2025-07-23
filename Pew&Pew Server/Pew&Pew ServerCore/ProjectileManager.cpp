#include "pch.h"
#include "ProjectileManager.h"

void ProjectileManager::AddProjectile(const std::shared_ptr<Projectile>& projectile)
{
	std::unique_lock lock{ _mutex };
	static int projectileId{ 64 };
	_projectiles.insert(std::make_pair(projectile->GetId(), projectile));
}

void ProjectileManager::RemoveProjectile(int projectileId)
{
	std::unique_lock lock{ _mutex };
	_projectiles.erase(projectileId);
}

std::shared_ptr<Projectile> ProjectileManager::GetProjectile(int projectileId) const
{
	std::shared_lock lock{ _mutex };
	auto it = _projectiles.find(projectileId);
	return (it != _projectiles.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Projectile>> ProjectileManager::GetProjectileList() const
{
	std::shared_lock lock{ _mutex };

	std::vector<std::shared_ptr<Projectile>> projectileList;
	projectileList.reserve(_projectiles.size());

	for (const auto& [id, projectile] : _projectiles) {
		projectileList.push_back(projectile);
	}

	return projectileList;
}

void ProjectileManager::Update(float deltaTime)
{
	for (auto& projectile : GetProjectileList()) {
		projectile->Update(deltaTime);
	}
}