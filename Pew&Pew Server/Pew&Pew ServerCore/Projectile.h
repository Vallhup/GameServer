#pragma once

struct vec3;
class Character;

class Projectile
{
	const float PROJECTILE_SPEED{ 0.5f };

public:
	Projectile(int id, int ownerId, vec3 pos, vec3 dir, int dmg);

public:
	void Update(float deltaTime, Service* service);
	bool CheckCollision(const Character& character) const;

	void SetIntactive(Service* service);

	int GetId() const { return _id; }
	int GetOwnerId() const { return _ownerId; }
	vec3 GetPosition() const { return _pos; }
	int GetDamage() const { return _damage; }

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
};

