#include "pch.h"
#include "Projectile.h"

Projectile::Projectile(int ownerId, vec3 pos, float dir, float speed, int dmg)
	: _ownerId(ownerId), _pos(pos), _speed(speed), _damage(dmg)
{
	_lifeTime = 0.0f;
	_maxLifeTime = 3.0f;

	_moveVector.x = cosf(dir) * speed;
	_moveVector.z = sinf(dir) * speed;
	_moveVector.y = 0.0f;

	_radius = 0.3f;
	_isActive = true;
}

void Projectile::Update(float deltaTime)
{
	_pos += _moveVector * deltaTime;
	_lifeTime += deltaTime;
	if (_lifeTime > _maxLifeTime) {
		_isActive = false;
	}
}

bool Projectile::CheckCollision(const Character& character) const
{
	vec3 charPos = character.GetPosition();
	vec3 deltaPos = _pos - charPos;

	// 오랜만에 수학할라니까 머리 깨지겠다...
	// 대충 충돌체크 한다는 코드...
}
