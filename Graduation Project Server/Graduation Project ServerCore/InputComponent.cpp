#include "pch.h"
#include "InputComponent.h"

void InputComponent::HandleInput(const Protocol::CS_INPUT_PACKET& packet)
{
	if (packet.inputtype() == Protocol::InputType::KeyDown) {
		_keyState.set((size_t)packet.key());
	}

	else {
		_keyState.reset((size_t)packet.key());
	}
}
