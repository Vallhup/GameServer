#pragma once

class Character
{
public:
	Character(int id, const std::string& name);

public:
	void Move(float newX, float newY, float newZ, float newRotation);
	void Attack(float nowTime);
	void TakeDamage(int damage);

private:
	int _id;
	std::string _name;

	float _x, _y, _z;
	float _rotation;

	int _hp;
	bool _isAlive;

	int _attackDamage;
	float _lastAttackTime;
	float _attackCooldown;
};

