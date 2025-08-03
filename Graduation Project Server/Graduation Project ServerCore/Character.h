#pragma once

class Character : public DynamicGameObject {
public:
	Character() = default;
	virtual ~Character() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Move(float deltaTime) override;
	virtual void TakeDamage(int damage) override;
	virtual void Die() override;
	virtual void Revive() override;
};

