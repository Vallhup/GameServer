#include "pch.h"
#include "Action.h"

/*---------------[ AttackAction ]---------------*/

void AttackAction::Start()
{
	_timer = 0.0f;
	_duration = ATTACK_DURATION;
	_finished = false;

	// Attack Packet Send
	if (auto trComp = _owner.GetComponent<TransformComponent>()) {
		_owner.GetInstance()->BroadCast(PacketFactory::SCAttackPacket(_owner.GetId(), trComp->GetAngle()));
	}

	// 공격 HitBox 활성화 
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
	// 공격 HitBox 비활성화
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
	const vec3 dir = GetForcedVelocity().Normalize();
	Protocol::Vec3 netDir;
	netDir.set_x(dir.x);
	netDir.set_y(dir.y);
	netDir.set_z(dir.z);

	_owner.GetInstance()->BroadCast(PacketFactory::SCDodgePacket(_owner.GetId(), netDir));

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

	if (auto trComp = _owner.GetComponent<TransformComponent>()) {
		const float angle = trComp->GetAngle();
		const vec3 forward = vec3{
			-sin(angle),
			0.0f,
			-cos(angle)
		}.Normalize();

		trComp->Translate(forward * DODGE_SPEED * deltaTime, false);
	}
}

void DodgeAction::End()
{
	// TODO : HitBox 활성화
	if (auto colComp = _owner.GetComponent<CollisionComponent>()) {
		colComp->Activate(CollisionType::Hurt);
	}
}

bool DodgeAction::CanMove() const
{
	return true;
}

const vec3 DodgeAction::GetForcedVelocity() const
{
	return vec3{ 0, 0, 0 };
}

/*---------------[ ParryAction ]---------------*/

void ParryAction::Start()
{
	_timer = 0.0f;
	_duration = PARRY_DURATION;
	_finished = false;

	// Parry Packet Send
	//_owner.GetInstance()->BroadCast(/* Parry Packet */);

	// TODO : 무적(패리) 처리
}

void ParryAction::Update(float deltaTime)
{
	_timer += deltaTime;
	if (_timer >= _duration) {
		_finished = true;
	}
}

void ParryAction::End()
{
	// TODO : 패리 상태 종료
}