#include "pch.h"
#include "InputComponent.h"

void InputComponent::HandleInput(const TestInputPacket& packet)
{
	if (packet.inputType == (char)TestInputType::KeyDown) {
		_keyState.set((size_t)packet.key);
	}

	else {
		_keyState.reset((size_t)packet.key);
	}
}
