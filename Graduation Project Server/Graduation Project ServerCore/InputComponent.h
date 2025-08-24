#pragma once

class InputComponent : public IComponent, public IInputable {

public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, Instance& instance)
		: IComponent(owner, instance) {}
	virtual ~InputComponent() = default;

public:	
	virtual void HandleInput(const Protocol::CS_INPUT_PACKET& packet) override;
	virtual void InputEnable() override { Enable(); }

public:
	bool IsHold(Protocol::Input key) const { return _keyState.test((size_t)key); }

private:
	InputSystem::KeyState _keyState;
};