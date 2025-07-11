#include "pch.h"
#include "Character.h"

Character::Character(int id, const std::string& name) : _id(id), _name(name)
{
	_x = _y = _z = 0.0f;
	_rotation = 0.0f;

	_hp = 100;
	_isAlive = true;

	_attackDamage = 10;
	_lastAttackTime = 0.0f;
	_attackCooldown = 1.0f;
}

void Character::Move(float newX, float newY, float newZ, float newRotation)
{
	_x = newX;
	_y = newY;
	_z = newZ;

	_rotation = newRotation;
}

void Character::Attack(float nowTime)
{
	if (not _isAlive) {
		return;
	}

	if ((nowTime - _lastAttackTime) >= _attackCooldown) {
		_lastAttackTime = nowTime;

		// 표창 생성, Service에 표창 등록 
		// (어떻게 할 지 고민중 계속 동적 생성할 지 or Object Pool 만들어서 관리할 지)
	}
}

void Character::TakeDamage(int damage)
{
	if (not _isAlive) {
		return;
	}

	_hp -= damage;
	if (_hp < 0) {
		_hp = 0;
		_isAlive = false;
	}
}
