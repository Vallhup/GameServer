#include "pch.h"
#include "MovementComponent.h"
#include "TickSystem.h"

MovementComponent::MovementComponent(GameObject& owner, Instance& instance) : IComponent(owner, instance)
{
	_velocity = { 0.0f, 0.0f, 0.0f };
	_maxSpeed = 0.0f;
}

void MovementComponent::Tick(float deltaTime)
{
	if (auto* trComp = _owner.GetComponent<TransformComponent>()) {
		ClampSpeed();
		trComp->Translate({ _velocity * deltaTime });
	}
}

void MovementComponent::ClampSpeed()
{
	const float speed2 = _velocity.DistanceSq();

	if (speed2 > _maxSpeed) {
		const float invLen = 1.0f / sqrtf(speed2);
		const float scale = invLen * _maxSpeed;
		_velocity = _velocity * scale;
	}

	// 1. invLen
	//  - 속도 벡터 _velocity의 길이의 역수
	//  - _velocity * invLen = 단위 방향 벡터
	//  - 속도 벡터에서 움직이는 방향을 추출하는 과정
	// 
	// 2. candidate
	//  - 얼마나 줄여야 할지
	//  - 현재 방향을 유지한채로 속도를 _maxSpeed에 맞도록 축소하는 비율
	// 
	// 3. scale
	//  - 실제로 적용할 scale
	//  - candidate가 1이상이면 줄일 필요 없음
	//
	// Branch Less로 설계하면
	//  1) speed가 0에 근접할 정도로 작거나
	//  2) candidaterk 1이상인 경우 (speed가 _maxSpeed보다 작은 경우)
	// 를 고려해 줘야함 (나중에 속도 계산 더 복잡해지면 해볼 예정...?)
}

void MovementComponent::OnRegister()
{
	_instance.GetScheduler().Register(this);
}

void MovementComponent::OnDeregister()
{
	_instance.GetScheduler().Deregister(this);
}