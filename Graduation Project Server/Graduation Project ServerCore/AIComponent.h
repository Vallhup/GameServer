#pragma once

// FSM? BT?
// 구현 방식 고민 중
// 최종보스 생각하면 BT정도 해도 되려나 싶기도 하고?
//
// Python은 의사결정만 담당하도록
// 실제 FSM or BT는 CPP에서 유지


class AIComponent : public IComponent {
public:
	AIComponent() = delete;
	AIComponent(GameObject& owner, Instance& instance) : IComponent(owner, instance) {}
	virtual ~AIComponent() = default;

private:
	virtual void OnRegister() override;
	virtual void OnDeregister() override;
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;

private:
};

