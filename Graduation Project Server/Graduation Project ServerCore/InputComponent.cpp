#include "pch.h"
#include "InputComponent.h"

void InputComponent::HandleInput(const Protocol::CS_INPUT_PACKET& packet)
{
	auto rawKey = packet.key();
	if (rawKey >= Protocol::MOVE_FRONT and rawKey <= Protocol::MOVE_RIGHT) {
		auto key = static_cast<size_t>(rawKey) - 1;

		(packet.inputtype() == Protocol::InputType::KeyDown) ? 
			_keyState.set(key) : _keyState.reset(key);
	}

	else {
		LOG_DBG("key >= _keyState.size() : key = %d", rawKey);
	}
}

void InputComponent::OnRegister()
{
	_instance.GetInputSystem().Register(_id, this);
}

void InputComponent::OnDeregister()
{
	_instance.GetInputSystem().Deregister(_id);
}
