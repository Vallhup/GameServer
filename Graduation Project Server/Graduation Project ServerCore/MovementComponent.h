#pragma once

class MovementComponent : public IComponent, public ITickable {
public:
	MovementComponent() = delete;
	MovementComponent(GameObject& owner, Instance& instance);
	virtual ~MovementComponent() = default;

public:
	virtual void Tick(float deltaTime) override;
	virtual bool TickEnable() override { return Enable(); }

private:
	virtual void OnRegister() override { _instance.GetScheduler().Register(this); }
	virtual void OnDeregister() override { _instance.GetScheduler().Deregister(this); }
	virtual void OnActivate() override {}
	virtual void OnDeactivate() override {}

	void ClampSpeed();

private:
	vec3 _velocity;
	float _maxSpeed;
};