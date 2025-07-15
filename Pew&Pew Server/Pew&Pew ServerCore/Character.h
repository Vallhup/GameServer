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
};

class Character
{
public:
	Character(int id, const std::string& name);

public:
	bool Move(float deltaTime);
	void Attack(float nowTime);
	void TakeDamage(int damage);

	int GetId() const { return _id; }
	int GetHp() const { return _hp; }
	bool GetRun() const { return _isRun; }
	float GetAngle() const { return _angle; }
	vec3 GetPosition() const { return _pos; }
	
	void SetInput(float angle, char direction, bool isRun) { _angle = angle; _direction = direction; _isRun = isRun; }

private:
	int _id;
	std::string _name;

	char _direction{ -1 };
	bool _isRun{ false };

	vec3 _pos;
	float _angle;

	int _hp;
	bool _isAlive;

	int _attackDamage;
	float _lastAttackTime;
	float _attackCooldown;
};

