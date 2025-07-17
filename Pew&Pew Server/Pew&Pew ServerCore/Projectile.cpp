#include "pch.h"
#include "Projectile.h"

Projectile::Projectile(int id, int ownerId, vec3 pos, vec3 dir, int dmg)
	: _id(id), _ownerId(ownerId), _pos(pos), _damage(dmg)
{
	_speed = PROJECTILE_SPEED;

	_lifeTime = 0.0f;
	_maxLifeTime = 3.0f;

	_moveVector.x = dir.x * _speed;
	_moveVector.z = dir.z * _speed;
	_moveVector.y = 0.0f;

	_radius = 0.3f;
	_isActive = true;
}

void Projectile::Update(float deltaTime, Service* service)
{
	_pos += _moveVector * deltaTime;
	_lifeTime += deltaTime;
	if (_lifeTime > _maxLifeTime) {
		_isActive = false;
		service->RemoveProjectile(_id);
		service->BroadCast(PacketFactory::SCRemovePacket(*this));
	}
}

bool Projectile::CheckCollision(const Character& character) const
{
	auto [charMin, charMax] = character.GetCollisionRange();

	
}
