#include "pch.h"
#include "InputComponent.h"

void InputComponent::Update(float deltaTime)
{
	while (not _inputQueue.empty()) {
		Protocol::InputPayload payload = _inputQueue.front();
		_inputQueue.pop();

		switch (payload.payload_case()) {
		case Protocol::InputPayload::kMove: {
			if (auto mvComp = _owner.GetComponent<MovementComponent>()) {
				mvComp->SetMovePayload(payload);
			}
			break;
		}
		case Protocol::InputPayload::kAction: {
			break;
		}
		}
	}
}

void InputComponent::Enqueue(const Protocol::CS_INPUT_PACKET& data)
{
	Protocol::InputPayload payload;
	payload.CopyFrom(data.payload());
	_inputQueue.push(std::move(payload));
}
