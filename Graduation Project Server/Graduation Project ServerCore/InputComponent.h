#pragma once

#include "GameObject.h"
#include "Protocols/Protocol.pb.h"

class InputComponent : public IComponent {

public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, Instance* instance)
		: IComponent(owner, instance) {}
	virtual ~InputComponent() = default;

public:
	virtual void LogicUpdate(float deltaTime) override;
	void Enqueue(Protocol::CS_INPUT_PACKET& data);

private:
	std::queue<Protocol::CS_INPUT_PACKET> _inputQueue;
};