#pragma once

// 1. Start
//  - 멤버 변수 초기화
//  - 각 Action에 따른 애니메이션 출력 위한 Packet Send
//  - 각 Action에 따른 판정 처리 
//    ex) 공격 - HitBox 활성화 / 회피 - 무적처리 등등
//
// 2. Update
//  - 시간 누적 및 종료 여부 판정
//
// 3. End
//  - 후 처리 (Start때 시작된 판정 처리 종료)

class IAction {
public:
	IAction() = delete;
	IAction(GameObject& owner) 
		: _owner(owner), _timer(0.0f), _duration(0.0f), _finished(false) {}
	virtual ~IAction() = default;

public:
	virtual void Start() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void End() = 0;
	virtual bool IsFinished() const { return _finished; }

protected:
	float _timer;
	float _duration;
	bool  _finished;

	GameObject& _owner;
};

// 공격 구현 
//
// 1. 공격 방향
//  - 캐릭터 forward 기준
//  - 카메라 forward 기준
// 
// 2. Hitbox 생성
//  - GameObject에 저장해놓고 특정 Action에서만 활성/비활성화
//  - 각 Action마다 동적으로 생성/삭제

class AttackAction : public IAction {
	// 공격 모션 시간
	// 추후 캐릭터 늘어나면 바뀔 예정
	static constexpr float ATTACK_DURATION{ 1.0f };

public:
	AttackAction() = delete;
	AttackAction(GameObject& owner) : IAction(owner) { Start(); }
	virtual ~AttackAction() = default;

public:
	virtual void Start() override;
	virtual void Update(float deltaTime) override;
	virtual void End() override;
};

class DodgeAction : public IAction {
	// 회피 모션(무적) 시간
	// 얘는 캐릭터 늘어나도 다 똑같겠지?
	static constexpr float DODGE_DURATION{ 1.0f };

public:
	DodgeAction() = delete;
	DodgeAction(GameObject& owner) : IAction(owner) { Start(); }
	virtual ~DodgeAction() = default;

public:
	virtual void Start() override;
	virtual void Update(float deltaTime) override;
	virtual void End() override;
};

