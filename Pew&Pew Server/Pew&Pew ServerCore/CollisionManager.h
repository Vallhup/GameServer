#pragma once

class ICollisionManager {
public:
	static bool SphereAABBCollision(const vec3& sphereCenter, float sphereRadius, const vec3& minAABB, const vec3& maxAABB);

public:
	virtual ~ICollisionManager() = default;

public:
	virtual std::vector<std::pair<std::shared_ptr<Projectile>, std::shared_ptr<Character>>>
		GetCollisionList(const std::vector<std::shared_ptr<Projectile>>& projectiles, const std::vector<std::shared_ptr<Character>>& characters) = 0;
};

class CollisionManager : public ICollisionManager
{
public:
	CollisionManager() = default;
	virtual ~CollisionManager() override = default;

public:
	virtual std::vector<std::pair<std::shared_ptr<Projectile>, std::shared_ptr<Character>>>
		GetCollisionList(const std::vector < std::shared_ptr<Projectile>>& projectiles, const std::vector<std::shared_ptr<Character>>& characters) override;
};