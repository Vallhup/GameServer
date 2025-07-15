#include "pch.h"
#include "Character.h"

Character::Character(int id, const std::string& name) : _id(id), _name(name)
{
	_pos = { -37.3051f, 0.0f, 42.5001f };
	_rotation = 0.0f;

	_hp = 100;
	_isAlive = true;

	_attackDamage = 10;
	_lastAttackTime = 0.0f;
	_attackCooldown = 1.0f;
}

bool Character::Move(float deltaTime)
{
	if (_direction < 0 or _direction >= 8) {
		return false;
	}

	float baseMove = (_isRun ? 3.0f : 1.5f);
	float moveDistance = baseMove * deltaTime;

	static constexpr vec3 dirTable[8] = {
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f,  1.0f },
		{ -1.0f, 0.0f, 0.0f },
		{  1.0f, 0.0f, 0.0f },
		{ -1.0f, 0.0f,  -1.0f },
		{ 1.0f, 0.0f,  -1.0f },
		{ -1.0f, 0.0f,  1.0f },
		{ 1.0f, 0.0f,  1.0f }
	};

	vec3 moveVec = dirTable[_direction];

	if (_direction >= MoveDirection::UPLEFT) {
		float normal = sqrt(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
		if (normal > 0.0f) {
			moveVec = moveVec / normal;
		}
	}

	_pos += moveVec * moveDistance;

	return true;
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
