#pragma once

struct vec3;
class Character;

class Projectile
{
public:
	Projectile(int ownerId, vec3 pos, float dir, float speed, int dmg);

public:
	void Update(float deltaTime);
	bool CheckCollision(const Character& character) const;

private:
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

