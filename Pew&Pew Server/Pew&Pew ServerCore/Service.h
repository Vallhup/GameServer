#pragma once

struct vec3;

class Listener;
class Session;
class Character;
class Projectile;
class TimerManager;
class CollisionManager;

class Service : public std::enable_shared_from_this<Service>
{
public:
	Service();
	~Service();

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;

public:
	bool Init();
	void Run();
	void Stop();

	void AddProjectile(int sessionId, vec3 direction);
	void RemoveProjectile(int projId);

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

private:
	int GenerateSessionId();

	void AcceptSession();
	void CloseSession(int id);

	void LogicTick(float deltaTime);
	void NetworkTick(float deltaTime);

	void UpdateCharacters(float deltaTime);
	void UpdateProjectiles(float deltaTime);
	void CheckCollisions();

private:
	std::shared_ptr<Listener> _listener;
	std::shared_ptr<TimerManager> _timerManager;
	std::shared_ptr<CollisionManager> _collisionManager;

	std::unordered_map<int, std::shared_ptr<Session>> _sessions;
	std::unordered_map<int, std::shared_ptr<Character>> _characters;
	std::unordered_map<int, std::shared_ptr<Projectile>> _projectiles;

	std::shared_mutex _sessionMutex;
	std::shared_mutex _characterMutex;
	std::shared_mutex _projectileMutex;

	std::atomic<int> _nextProjectileId;

	std::vector<int> _reusableSessionIds;
	bool _running{ false };
};

