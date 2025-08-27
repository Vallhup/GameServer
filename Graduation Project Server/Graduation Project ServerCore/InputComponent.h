#pragma once

#include "GameObject.h"

class InputComponent : public IComponent, public IInputable {

public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, Instance& instance)
		: IComponent(owner, instance), IInputable(owner.GetId()) {}
	virtual ~InputComponent() = default;

public:	
	virtual void HandleInput(const Protocol::CS_INPUT_PACKET& packet) override;
	virtual void InputEnable() override { Enable(); }
	
public:
	bool IsKeyDown(Protocol::Input key) const { return _keyState.test((size_t)key); }

private:
	virtual void OnRegister() override;
	virtual void OnDeregister() override;
	virtual void OnActivate() override {}
	virtual void OnDeactivate() override {}

private:
	InputSystem::KeyState _keyState;
};