#include "pch.h"
#include "ActionManager.h"

ActionManager::ActionManager()
{
	LoadPolicy();
	LoadProfile();
}

void ActionManager::LoadAction(ActionType id, std::string_view path)
{
	if (_actionProfiles.contains(id)) return;

	auto anim = std::make_unique<ActionProfile>(LoadActionProfile(path));

#ifdef _DEBUG
	if (anim.get() != nullptr)
		std::cout << "Action Load Success: " << path << std::endl;
#endif

	_actionProfiles.try_emplace(id, std::move(anim));
}

const ActionPolicy& ActionManager::GetPolicy(ActionType action) const
{
	return _policies[ToIndex(action)];
}

const ActionPolicy& ActionManager::GetPolicy(ActionType action, EntityType entity, AttackType attack) const
{
	const ActionPolicy* out{ nullptr };
	if (out = Find(action, entity, attack))
		return *out;

	return ActionPolicy{};
}

const ActionProfile* ActionManager::GetActionMoveProfile(ActionType actionType) const
{
	auto it = _actionProfiles.find(actionType);
	if (it != _actionProfiles.end()) return it->second.get();
	return nullptr;
}

void ActionManager::LoadPolicy()
{
	// TEMP : 추후 데이터 구조 확정 및 툴 완성 후 분리
	_policies[ToIndex(ActionType::Dead)] = { 100, 150.f / 30.2013f, 0u, false, false };
	_policies[ToIndex(ActionType::Hit)] = { 90,  50.f / 30.6122f, Bit(ActionType::Dead), false, false };
	_policies[ToIndex(ActionType::Stun)] = { 85,  96.f / 30.3158f, Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false };
	_policies[ToIndex(ActionType::Parry)] = { 80,  54.f / 30.566f,  Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false };
	_policies[ToIndex(ActionType::Dodge)] = { 70,  50.f / 30.6122f, Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), true, false };
	_policies[ToIndex(ActionType::Attack)] = { 60,  40.f / 30.7692f, Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), true, false };
	_policies[ToIndex(ActionType::Guard)] = { 10,  std::numeric_limits<double>::infinity(), ~0u, false, true };
	_policies[ToIndex(ActionType::None)] = { 0,  0.f, ~0u, false, false };

	// Knight
	SetPolicy(ActionType::None, EntityType::Knight, AttackType::None,
		ActionPolicy{ 0,  0.f, ~0u, false, false });

	SetPolicy(ActionType::Guard, EntityType::Knight, AttackType::None,
		ActionPolicy{ 10,  std::numeric_limits<double>::infinity(), ~0u, false, true });

	SetPolicy(ActionType::Attack, EntityType::Knight, AttackType::None,
		ActionPolicy{ 60,  40.f / 30.7692f, Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), true, false });

	SetPolicy(ActionType::Dodge, EntityType::Knight, AttackType::None,
		ActionPolicy{ 70,  50.f / 30.6122f, Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), true, false });

	SetPolicy(ActionType::Parry, EntityType::Knight, AttackType::None,
		ActionPolicy{ 80,  54.f / 30.566f,  Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false });

	SetPolicy(ActionType::Stun, EntityType::Knight, AttackType::Light,
		ActionPolicy{ 85,  96.f / 30.3158f, Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false });

	SetPolicy(ActionType::Hit, EntityType::Knight, AttackType::None,
		ActionPolicy{ 90,  50.f / 30.6122f, Bit(ActionType::Dead), false, false });

	SetPolicy(ActionType::Dead, EntityType::Knight, AttackType::None,
		ActionPolicy{ 100, 150.f / 30.2013f, 0u, false, false });

	
	// FInal Boss
	SetPolicy(ActionType::None, EntityType::Final_Boss, AttackType::None, 
		ActionPolicy{ 0,  0.f, ~0u, false, false });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::JumpSlash, 
		ActionPolicy{ 60, 50.f / 30.6122f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false});

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::MultiSlash, 
		ActionPolicy{ 60, 93.f / 30.3261f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::DashSlash, 
		ActionPolicy{ 60, 56.f / 30.5455f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::Thrust, 
		ActionPolicy{ 60, 56.f / 30.5455f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::Slash, 
		ActionPolicy{ 60, 47.f / 30.6522f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false });

	SetPolicy(ActionType::Stun, EntityType::Final_Boss, AttackType::None, 
		ActionPolicy{ 85, 201.f / 30.15f, Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false });

	SetPolicy(ActionType::Hit, EntityType::Final_Boss, AttackType::None,
		ActionPolicy{ 90, 35.f / 30.8824f, Bit(ActionType::Dead), false, false });

	SetPolicy(ActionType::Dead, EntityType::Final_Boss, AttackType::None, 
		ActionPolicy{ 100, 166.f / 30.1818f, 0u, false, false });

}

void ActionManager::LoadProfile()
{
	// TEMP : 추후 데이터 구조 확정 및 툴 완성 후 분리
	ActionProfile attack{
		{
			{
				0.0f,
				11.0f / 40.0f,
				11.54f / 100.f,
				true
			},

			{
				11.0f / 40.0f,
				20.0f / 40.0f,
				26.782f / 100.f,
				true
			},

			{
				20.0f / 40.0f,
				1.0f,
				57.311f / 100.f,
				true
			},
		}
	};

	auto attackPtr = std::make_unique<ActionProfile>(attack);
	_actionProfiles.try_emplace(ActionType::Attack, std::move(attackPtr));

	ActionProfile dodge{
		{
			{
				0.0f,
				7.0f / 50.0f,
				22.796f / 100.f,
				true
			},

			{
				7.0f / 50.0f,
				40.0f / 50.0f,
				319.144f / 100.f,
				true
			},

			{
				40.0f / 50.0f,
				1.0f,
				6.03f / 100.f,
				true
			}
		}
	};

	auto dodgePtr = std::make_unique<ActionProfile>(dodge);
	_actionProfiles.try_emplace(ActionType::Dodge, std::move(dodgePtr));
}

ActionProfile ActionManager::LoadActionProfile(std::string_view path)
{
	return ActionProfile();
}

void ActionManager::SetPolicy(ActionType action, EntityType entity, AttackType attack, const ActionPolicy& policy)
{
	_policyTable[Index(action, entity, attack)] = policy;
}

const ActionPolicy* ActionManager::Find(ActionType action, EntityType entity, AttackType attack) const
{
	const auto& policy = _policyTable[Index(action, entity, attack)];
	if (policy.IsValid()) return nullptr;
	return &policy;
}
