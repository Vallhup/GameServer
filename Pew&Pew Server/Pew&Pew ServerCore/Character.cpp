#include "pch.h"
#include "Character.h"

Character::Character(int id, const std::string& name) : _id(id), _name(name)
{
	_pos = DEFAULT_POS;
	_angle = 0.0f;
	_angleChange = false;

	_hp = MAX_HP;
	_isAlive = true;
	_isDeadProcessed = false;

	_version = 0;
	_lastSentVersion = 0;
}

void Character::Update(float deltaTime)
{
	Move(deltaTime);
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
		{ -1.0f, 0.0f, -1.0f },
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
	_version++;

	return true;
}

void Character::TakeDamage(int damage)
{
	if (not _isAlive) {
		return;	
	}

	_hp -= damage;
	if (_hp <= 0) {
		Death();
	}
}

std::optional<vec3> Character::GetNextProjectile(float nowTime)
{
	std::lock_guard lock{ _attackSeqMutex };

	if (not _attackSeq.has_value()) {
		return std::nullopt;
	}

	auto& seq = _attackSeq.value();
	if (seq.attackTimes.empty()) {
		_attackSeq.reset();
		return std::nullopt;
	}

	if (nowTime >= seq.attackTimes.front()) {
		seq.attackTimes.erase(seq.attackTimes.begin());

		if (seq.attackTimes.empty()) {
			_attackSeq.reset();
		}

		return seq.direction;
	}

	return std::nullopt;
}

std::pair<vec3, vec3> Character::GetCollisionBox() const
{
	const vec3 offsetMin{ SIZE_X / 2, 0, SIZE_Z / 2 };
	const vec3 offsetMax{ SIZE_X / 2, SIZE_Y, SIZE_Z / 2 };

	return { _pos - offsetMin, _pos + offsetMax };
}

void Character::SetInput(float angle, char direction, bool isRun)
{
	if (_angle != angle) {	
		_angle = angle;
		_angleChange = true;
	}
	
	_direction = direction; 
	_isRun = isRun;
}

void Character::SetAttackSequence(float nowTime, const vec3& dir)
{
	float attackOffset =
		_isRun	 ? 0.43f :
		IsMove() ? 0.56f :
				   0.45f;

	std::lock_guard lock{ _attackSeqMutex };	

	if (_attackSeq.has_value()) {
		return;
	}

	std::vector<float> attackTimes(NUMBER_OF_ATTACK);
	std::generate_n(attackTimes.begin(), NUMBER_OF_ATTACK,
		[n = 0, &nowTime, &attackOffset, this]() mutable
		{
			return nowTime + attackOffset + (n++) * INTERVAL_OF_ATTACK;
		});

	_attackSeq = AttackSequence{ dir, attackTimes };
}

bool Character::VersionCheckAndChange()
{
	if (_version != _lastSentVersion) {
		_lastSentVersion = _version;
		return true;
	}

	return false;
}

void Character::Death()
{
	_hp = 0;
	_isAlive = false;

	_direction = -1;
	_isRun = false;
	_angleChange = false;
}

void Character::Revive()
{
	_pos = DEFAULT_POS;
	_angle = 0.0f;
	_angleChange = false;

	_hp = MAX_HP;
	_isAlive = true;
	_isDeadProcessed = false;

	_direction = -1;
	_isRun = false;
}