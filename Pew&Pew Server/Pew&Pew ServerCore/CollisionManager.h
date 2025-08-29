#pragma once

class ICollisionManager {
public:
	static bool SphereAABBCollision(const vec3& sphereCenter, float sphereRadius, const vec3& minAABB, const vec3& maxAABB);
	static bool AABBCollision(const vec3& minA, const vec3& maxA, const vec3& minB, const vec3& maxB);

public:
	virtual ~ICollisionManager() = default;

public:
	virtual std::vector<std::pair<std::shared_ptr<Projectile>, std::shared_ptr<Character>>>
		GetCollisionList(const std::vector<std::shared_ptr<Projectile>>& projectiles, const std::vector<std::shared_ptr<Character>>& characters) = 0;
};

class CollisionManager : public ICollisionManager
{
public:
	CollisionManager() = delete;
	CollisionManager(IGameContext& gameCtx) : _gameCtx(gameCtx) {}
	virtual ~CollisionManager() override = default;

	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;

	CollisionManager(CollisionManager&&) = delete;
	CollisionManager& operator=(CollisionManager&&) = delete;

public:
	virtual std::vector<std::pair<std::shared_ptr<Projectile>, std::shared_ptr<Character>>>
		GetCollisionList(const std::vector < std::shared_ptr<Projectile>>& projectiles, const std::vector<std::shared_ptr<Character>>& characters) override;

private:
	IGameContext& _gameCtx;
};