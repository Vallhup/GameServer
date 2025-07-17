#include "pch.h"
#include "Character.h"

Character::Character(int id, const std::string& name) : _id(id), _name(name)
{
	_pos = { -37.3051f, 0.0f, 42.5001f };
	_angle = 0.0f;
	_angleChange = false;

	_hp = 100;
	_isAlive = true;
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

void Character::Attack(float nowTime, Service* service)
{
	std::unique_lock lock{ _attackSeqMutex };

	if (not _attackSeq.has_value() or not _isAlive) {
		return;
	}

	auto& seq = _attackSeq.value();

	while (not seq.attackTimes.empty() and nowTime >= seq.attackTimes.front()) {
		service->AddProjectile(_id, seq.direction);
		seq.attackTimes.erase(seq.attackTimes.begin());
	}

	if (seq.attackTimes.empty()) {
		_attackSeq.reset();
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
	std::unique_lock lock{ _attackSeqMutex };

	if (_attackSeq.has_value()) {
		return;
	}

	std::vector<float> attackTimes;
	attackTimes.reserve(NUMBER_OF_ATTACK);

	std::generate_n(attackTimes.begin(), NUMBER_OF_ATTACK,
		[n = 0, &nowTime, this]() mutable
		{
			return nowTime + (n++) * INTERVAL_OF_ATTACK;
		});

	_attackSeq = AttackSequence{ dir, attackTimes };
}
