#pragma once

class ActionComponent : public IComponent {
public:
	ActionComponent() = delete;
	ActionComponent(GameObject& owner, Instance* instance)
		: IComponent(owner, instance), _currentAction(nullptr) {}
	virtual ~ActionComponent() = default;

public:
	virtual void LogicUpdate(float deltaTime) override;

	IAction* GetCurrentAction() const { return _currentAction.get(); }

	void StartAttack();
	void StartDodge();
	void StartParry();

private:
	std::unique_ptr<IAction> _currentAction;
};