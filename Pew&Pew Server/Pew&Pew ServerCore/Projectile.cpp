#include "pch.h"
#include "Projectile.h"

Projectile::Projectile(int id, int ownerId, vec3 charPos, vec3 dir, int dmg)
	: _id(id), _ownerId(ownerId), _damage(dmg)
{
	float angle = atan2(dir.x, dir.z) + PI;
	_pos = charPos;
	_pos.x += cos(angle) * 0.2f;  
	_pos.z -= sin(angle) * 0.2f;
	_pos.y = 0.45f;
	
	_speed = PROJECTILE_SPEED;
	_lifeTime = 0.0f;
	_maxLifeTime = MAX_LIFE_TIME;
	_moveVector.x = dir.x * _speed;
	_moveVector.z = dir.z * _speed;
	_moveVector.y = 0.0f;
	_radius = PROJECTILE_RADIUS;
	_isActive = true;

	_version = 0;
	_lastSentVersion = 0;
}

void Projectile::Update(float deltaTime)
{
	_pos += _moveVector * deltaTime;
	_lifeTime += deltaTime;
	_version++;

	if (_pos.x < -14.9f or _pos.x > 14.9f or _pos.z < -15.0f or _pos.z > 14.8f) {
		SetInactive();
	}

	if (_lifeTime > _maxLifeTime) {
		SetInactive();
	}
}

bool Projectile::CheckCollision(const Character& character) const
{
	auto [charMin, charMax] = character.GetCollisionBox();
	return CollisionManager::SphereAABBCollision(_pos, _radius, charMin, charMax);
}

void Projectile::SetInactive()
{
	_isActive = false;
}

bool Projectile::VersionCheckAndChange()
{
	if (_version != _lastSentVersion) {
		_lastSentVersion = _version;
		return true;
	}

	return false;
}
