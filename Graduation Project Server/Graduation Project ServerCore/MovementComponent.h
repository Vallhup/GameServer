#pragma once

#include "GameObject.h"

class MovementComponent : public IComponent, public ITickable {
public:
	MovementComponent() = delete;
	MovementComponent(GameObject& owner, Instance& instance);
	virtual ~MovementComponent() = default;

public:
	virtual void Tick(float deltaTime) override;
	virtual bool TickEnable() override { return Enable(); }

public:
	void SetVelocity(const vec3& velocity) 
	{ 
		_velocity = velocity; 
		/*LOG_DBG("MovementComponent[%d] SetVelocity : velocity={ %.2f, %.2f, %.2f }", 
			_owner.GetId(), velocity.x, velocity.y, velocity.z);*/
	}

private:
	virtual void OnRegister() override;
	virtual void OnDeregister() override;
	virtual void OnActivate() override {}
	virtual void OnDeactivate() override {}

private:
	vec3 _velocity;
};