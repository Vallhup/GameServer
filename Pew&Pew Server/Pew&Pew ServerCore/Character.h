#pragma once

struct vec3 {
	float x;
	float y;
	float z;

	vec3 operator+(const vec3& other) const
	{
		return vec3{ x + other.x, y + other.y, z + other.z };
	}

	vec3 operator-(const vec3& other) const
	{
		return vec3{ x - other.x, y - other.y, z - other.z };
	}

	vec3 operator*(float other) const
	{
		return vec3{ x * other, y * other, z * other };
	}

	vec3 operator/(float other) const
	{
		return vec3{ x / other, y / other, z / other };
	}

	vec3& operator+=(const vec3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	float DistanceSq(const vec3& other) const
	{
		vec3 deltaVec = *this - other;
		return powf(deltaVec.x, 2) + powf(deltaVec.y, 2) + powf(deltaVec.z, 2);
	}

	vec3 Cross(const vec3& other) const
	{
		return {
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		};
	}

	vec3 Normalize() const
	{
		float len = sqrtf(powf(x, 2) + powf(y, 2) + powf(z, 2));
		if (len == 0) return { 0, 0, 0 };
		return *this / len;
	}
};

class Character : public std::enable_shared_from_this<Character>
{
	struct AttackSequence {
		vec3 direction;
		std::vector<float> attackTimes;
	};

	const int NUMBER_OF_ATTACK{ 3 };
	const float INTERVAL_OF_ATTACK{ 0.15f };

	const float SIZE_X{ 0.5f };
	const float SIZE_Y{ 0.95f };
	const float SIZE_Z{ 0.4f };

	const int MAX_HP{ 100 };
	const vec3 DEFAULT_POS{ -37.3051f, 0.0f, 42.5001f };

public:
	Character(int id, const std::string& name);

public:
	void Update(float deltaTime);

public:
	bool Move(float deltaTime);
	void TakeDamage(int damage);
	
	void Death();
	void Revive();

	int GetId() const { return _id; }
	int GetHp() const { return _hp; }
	bool GetRun() const { return _isRun; }
	float GetAngle() const { return _angle; }
	bool GetAngleChange() const { return _angleChange; }
	vec3 GetPosition() const { return _pos; }
	long long GetVersion() const { return _version; }

	std::optional<vec3> GetNextProjectile(float nowTime);

	bool IsAlive() const { return _isAlive; }
	bool IsMove() const { return _direction >= 0 and _direction < 8; }
	
	std::pair<vec3, vec3> GetCollisionBox() const;
	
	void SetInput(float angle, char direction, bool isRun);
	void SetAttackSequence(float nowTime, const vec3& dir);
	void ResetAttackSequence() { std::lock_guard lock{ _attackSeqMutex }; _attackSeq.reset(); }
	void ResetAngleChange() { _angleChange = false; }

	bool VersionCheckAndChange();

private:
	int _id;
	std::string _name;

	char _direction{ -1 };
	bool _isRun{ false };

	vec3 _pos;
	float _angle;
	bool _angleChange;

	int _hp;
	bool _isAlive;

	long long _version;
	long long _lastSentVersion;

	std::optional<AttackSequence> _attackSeq;
	std::mutex _attackSeqMutex;
};