#pragma once

class AIComponent : public IComponent {
public:
	AIComponent() = delete;
	AIComponent(GameObject& owner, Instance* instance, AiType type, ScriptVM& scriptVM);
	virtual ~AIComponent() = default;

private:
	std::unique_ptr<IAiBehavior> _behavior;
};