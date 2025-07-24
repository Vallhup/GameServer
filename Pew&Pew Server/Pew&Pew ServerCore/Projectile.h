#pragma once

struct vec3;
class Character;

class Projectile
{
	static constexpr float PROJECTILE_SPEED{ 20.0f };
	static constexpr float MAX_LIFE_TIME{ 3.0f };
	static constexpr float PROJECTILE_RADIUS{ 0.3f };

public:
	Projectile() = delete;
	Projectile(int id, int ownerId, vec3 charPos, vec3 dir, int dmg);
	~Projectile() = default;

	Projectile(const Projectile&) = delete;
	Projectile& operator=(const Projectile&) = delete;

	Projectile(Projectile&&) = delete;
	Projectile& operator=(Projectile&&) = delete;

public:
	void Update(float deltaTime);
	bool CheckCollision(const Character& character) const;

	void SetInactive();

	int GetId() const { return _id; }
	int GetOwnerId() const { return _ownerId; }
	vec3 GetPosition() const { return _pos; }
	int GetDamage() const { return _damage; }

	bool IsActive() const { return _isActive; }

	bool VersionCheckAndChange();

private:
	int _id;
	int _ownerId;

	vec3 _pos;
	vec3 _moveVector;
	float _speed;
	
	int _damage;

	float _lifeTime;
	float _maxLifeTime;

	float _radius;
	
	bool _isActive;

	long long _version;
	long long _lastSentVersion;
};

