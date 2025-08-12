#pragma once

class InputComponent : public IComponent {
public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, IGameContext& gameCtx) : IComponent(owner, gameCtx) {}
	virtual ~InputComponent() = default;

public:

};

