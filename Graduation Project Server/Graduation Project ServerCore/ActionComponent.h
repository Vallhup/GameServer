#pragma once

class ActionComponent : public IComponent {
public:
	ActionComponent() = delete;
	ActionComponent(GameObject& owner, Instance* instance)
		: IComponent(owner, instance), _currentAction(nullptr) {}
	virtual ~ActionComponent() = default;

public:
	virtual void LogicUpdate(float deltaTime) override;

	// 이동 처리 어떻게 해야될 지 고민해야됨
	// ActionComponent에서 처리할 지 / Action에서 처리할 지
	void StartAttack();
	void StartDodge();

private:
	std::unique_ptr<IAction> _currentAction;
};