#pragma once

class AIComponent : public IComponent {
public:
	AIComponent() = delete;
	AIComponent(GameObject& owner, Instance& instance, AiType type, ScriptVM& scriptVM);
	virtual ~AIComponent() = default;

private:
	virtual void OnRegister() override;
	virtual void OnDeregister() override;
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;

private:

private:
	std::unique_ptr<IAiBehavior> _behavior;
};