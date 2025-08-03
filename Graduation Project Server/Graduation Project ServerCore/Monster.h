#pragma once

class Monster : public DynamicGameObject {
public:
	Monster() = default;
	virtual ~Monster() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Move(float deltaTime) override;
	virtual void TakeDamage(int damage) override;
	virtual void Die() override;
	virtual void Revive() override;
};

