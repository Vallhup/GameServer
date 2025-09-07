#include "pch.h"
#include "Action.h"

/*---------------[ AttackAction ]---------------*/

void AttackAction::Start()
{
	_timer = 0.0f;
	_duration = ATTACK_DURATION;
	_finished = false;

	// Attack Packet Send
	//_owner.GetInstance()->BroadCast(PacketFactory::SCAttackPacket(_owner.GetId(), ));

	// TODO : 공격 HitBox 활성화 
	if (auto colComp = _owner.GetComponent<CollisionComponent>()) {
		colComp->Activate(CollisionType::Attack);
	}
}

void AttackAction::Update(float deltaTime)
{
	_timer += deltaTime;
	if (_timer >= _duration) {
		_finished = true;
	}
}

void AttackAction::End()
{
	// TODO : 공격 HitBox 비활성화
	if (auto colComp = _owner.GetComponent<CollisionComponent>()) {
		colComp->Deactivate(CollisionType::Attack);
	}
}

/*---------------[ DodgeAction ]---------------*/

void DodgeAction::Start()
{
	_timer = 0.0f;
	_duration = DODGE_DURATION;
	_finished = false;

	// Dodge Packet Send
	//_owner.GetInstance()->BroadCast(/* Dodge Packet */);

	// TODO : 무적 처리 (HitBox 비활성화)
	if (auto colComp = _owner.GetComponent<CollisionComponent>()) {
		colComp->DeactivateAll();
	}
}

void DodgeAction::Update(float deltaTime)
{
	_timer += deltaTime;
	if (_timer >= _duration) {
		_finished = true;
	}
}

void DodgeAction::End()
{
	// TODO : HitBox 활성화
	if (auto colComp = _owner.GetComponent<CollisionComponent>()) {
		colComp->Activate(CollisionType::Hurt);
	}
}