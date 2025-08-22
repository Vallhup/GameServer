#pragma once

class InputComponent : public IComponent, public IInputable {
public:
	InputComponent() = delete;
	InputComponent(GameObject& owner, Instance& instance);
};