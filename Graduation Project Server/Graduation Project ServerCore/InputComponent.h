#pragma once

class InputComponent : public IComponent {
public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, Instance& instance) : IComponent(owner, instance) {}
	virtual ~InputComponent() = default;

public:
	void EnqueueCommand(const InputStruct::InputCommand& cmd);
	bool DequeueIntent(InputStruct::IntentEvent& out);

private:
	virtual void OnRegister() override;
	virtual void OnDeregister() override;
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;

private:
	concurrency::concurrent_queue<InputStruct::InputCommand> _commandQueue;

	InputStruct::InputState _state;
	std::deque<InputStruct::IntentEvent> _intentQueue;

	uint32_t _lastAcceptedSeq;
};