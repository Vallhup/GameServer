#pragma once

class MovementComponent : public IComponent {
public:
	MovementComponent() = delete;
	MovementComponent(GameObject& owner, IGameContext& gameCtx);
	virtual ~MovementComponent() = default;

public:
	virtual void Update(float deltaTime) override;

private:
	void ClampSpeed();

private:
	vec3 _velocity;
	float _maxSpeed;
};