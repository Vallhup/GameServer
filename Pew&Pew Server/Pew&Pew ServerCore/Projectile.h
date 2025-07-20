#pragma once

struct vec3;
class Character;

class Projectile
{
	const float PROJECTILE_SPEED{ 20.0f };

public:
	Projectile(int id, int ownerId, vec3 charPos, vec3 dir, int dmg);

public:
	bool Update(float deltaTime, Service* service);
	bool CheckCollision(const Character& character) const;

	void SetIntactive(Service* service);
	void SetDirtyFlag(bool flag) { _dirtyFlag = flag; }

	int GetId() const { return _id; }
	int GetOwnerId() const { return _ownerId; }
	vec3 GetPosition() const { return _pos; }
	int GetDamage() const { return _damage; }
	bool GetDirtyFlag() const { return _dirtyFlag; }

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
	bool _dirtyFlag;
};

