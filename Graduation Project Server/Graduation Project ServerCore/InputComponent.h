#pragma once

// 포기
// 내 능력에 비해 너무 많은 걸 건드려버렸다

class InputComponent : public IComponent {
public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, IGameContext& gameCtx) : IComponent(owner, gameCtx) {}
	virtual ~InputComponent() = default;

public:
	virtual void Update(float) override;

public:
	void EnqueueCommand(const InputStruct::InputCommand& cmd);
	bool DequeueIntent(InputStruct::IntentEvent& out);

private:
	concurrency::concurrent_queue<InputStruct::InputCommand> _commandQueue;

	InputStruct::InputState _state;
	std::deque<InputStruct::IntentEvent> _intentQueue;

	uint32_t _lastAcceptedSeq;
};