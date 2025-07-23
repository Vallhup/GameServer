#pragma once

struct vec3;
class Character;

class Projectile
{
	const float PROJECTILE_SPEED{ 20.0f };

public:
	Projectile(int id, int ownerId, vec3 charPos, vec3 dir, int dmg);

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

