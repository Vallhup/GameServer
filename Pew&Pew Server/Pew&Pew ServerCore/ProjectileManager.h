#pragma once

class IProjectileManager {
public:
	virtual ~IProjectileManager() = default;

public:
	virtual void AddProjectile(const std::shared_ptr<Projectile>& projectile) = 0;
	virtual void RemoveProjectile(int projectileId) = 0;

	virtual std::shared_ptr<Projectile> GetProjectile(int projectileId) const = 0;
	virtual std::vector<std::shared_ptr<Projectile>> GetProjectileList() const = 0;

	virtual void Update(float deltaTime) = 0;
};

class ProjectileManager : public IProjectileManager
{
public:
	ProjectileManager() = default;
	virtual ~ProjectileManager() = default;

public:
	virtual void AddProjectile(const std::shared_ptr<Projectile>& projectile) override;
	virtual void RemoveProjectile(int projectileId) override;

	virtual std::shared_ptr<Projectile> GetProjectile(int projectileId) const override;
	virtual std::vector<std::shared_ptr<Projectile>> GetProjectileList() const override;

	virtual void Update(float deltaTime) override;

private:
	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Projectile>> _projectiles;
};

