#pragma once

struct vec3 {
	float x;
	float y;
	float z;

	vec3 operator-(const vec3& other) const
	{
		return vec3{ x - other.x, y - other.y, z - other.z };
	}
};

class Character
{
public:
	Character(int id, const std::string& name);

public:
	void Move(vec3 newPos, float newRotation);
	void Attack(float nowTime);
	void TakeDamage(int damage);

	vec3 GetPosition() const { return _pos; }

private:
	int _id;
	std::string _name;

	vec3 _pos;
	float _rotation;

	int _hp;
	bool _isAlive;

	int _attackDamage;
	float _lastAttackTime;
	float _attackCooldown;
};

