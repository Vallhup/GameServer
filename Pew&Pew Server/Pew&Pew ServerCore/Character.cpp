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

void Character::Move(vec3 newPos, float newRotation)
{
	_pos = newPos;
	_rotation = newRotation;
}

void Character::Move(char direction, float rotation)
{
	// Temp : 이동 패킷 처리 자체는 되는데 이동 동기화가 제대로 안됨
	//        이동 로직 현재 Client 코드와 맞춰야 함

	static constexpr float moveDistance{ 0.005f };
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
	
	if (direction < 0 or direction >= 8) {
		return;
	}

	vec3 moveVec = dirTable[direction];

	if (direction >= MoveDirection::UPLEFT) {
		float normal = sqrt(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
		if (normal > 0.0f) {
			moveVec = moveVec / normal;
		}
	}

	_pos += moveVec * moveDistance;
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
