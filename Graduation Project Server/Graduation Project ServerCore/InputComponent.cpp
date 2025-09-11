#include "pch.h"
#include "InputComponent.h"

void InputComponent::LogicUpdate(float deltaTime)
{
	while (not _inputQueue.empty()) {
		Protocol::CS_INPUT_PACKET packet;
		if (_inputQueue.try_pop(packet)) {
			switch (packet.key()) {
			case Protocol::Input::MOVE: {
				if (auto mvComp = _owner.GetComponent<MovementComponent>()) {
					mvComp->SetMovePayload(packet.payload());
				}
				break;
			}
			case Protocol::Input::ATTACK: {
				if (auto actComp = _owner.GetComponent<ActionComponent>()) {
					actComp->StartDodge();
				}
				break;
			}
			case Protocol::Input::DODGE: {
				if (auto actComp = _owner.GetComponent<ActionComponent>()) {
					actComp->StartDodge();
				}
				break;
			}
			}
		}
	}
}

void InputComponent::Enqueue(Protocol::CS_INPUT_PACKET& data)
{
	_inputQueue.push(std::move(data));
}
