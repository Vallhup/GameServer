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
	_maxLifeTime = 3.0f;
	_moveVector.x = dir.x * _speed;
	_moveVector.z = dir.z * _speed;
	_moveVector.y = 0.0f;
	_radius = 0.3f;
	_isActive = true;
}

bool Projectile::Update(float deltaTime, Service* service)
{
	_pos += _moveVector * deltaTime;
	_lifeTime += deltaTime;

	if (_lifeTime > _maxLifeTime) {
		SetIntactive(service);
		return false;
	}

	return true;
}

bool Projectile::CheckCollision(const Character& character) const
{
	auto [charMin, charMax] = character.GetCollisionBox();
	return CollisionManager::SphereAABBCollision(_pos, _radius, charMin, charMax);
}

void Projectile::SetIntactive(Service* service)
{
	_isActive = false;
	service->RemoveProjectile(_id);
	service->BroadCast(PacketFactory::SCRemovePacket(*this));
}
