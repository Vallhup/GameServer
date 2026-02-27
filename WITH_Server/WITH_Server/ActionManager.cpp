#include "pch.h"
#include "ActionManager.h"

ActionManager::ActionManager()
{
	LoadPolicy();
	LoadProfile();
}

void ActionManager::LoadAction(ActionType id, std::string_view path)
{
	/*auto anim = std::make_unique<ActionProfile>(LoadActionProfile(path));

#ifdef _DEBUG
	if (anim.get() != nullptr)
		std::cout << "Action Load Success: " << path << std::endl;
#endif

	_actionProfiles.try_emplace(id, std::move(anim));*/
}

const ActionPolicy& ActionManager::GetPolicy(ActionType action, EntityType entity, AttackType attack) const
{
	const ActionPolicy* out{ nullptr };
	if (out = FindPolicy(action, entity, attack))
		return *out;

	return ActionPolicy{};
}

const ActionProfile* ActionManager::GetActionMoveProfile(ActionType action, EntityType entity, AttackType attack) const
{
	const ActionProfile* out{ nullptr };
	out = FindProfile(action, entity, attack);
	return out;
}

void ActionManager::LoadPolicy()
{
	// TEMP : 추후 데이터 구조 확정 및 툴 완성 후 분리
	// Knight
	SetPolicy(ActionType::None, EntityType::Knight, AttackType::None,
		ActionPolicy{ 0,  0.f, ~0u, false, false, false });

	SetPolicy(ActionType::Guard, EntityType::Knight, AttackType::None,
		ActionPolicy{ 10,  std::numeric_limits<double>::infinity(), ~0u, false, true, false });

	SetPolicy(ActionType::Attack, EntityType::Knight, AttackType::Light,
		ActionPolicy{ 60,  40.f / 30.7692f, Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), true, false, true });

	SetPolicy(ActionType::Dodge, EntityType::Knight, AttackType::None,
		ActionPolicy{ 70,  50.f / 30.6122f, Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), true, false, false });

	SetPolicy(ActionType::Parry, EntityType::Knight, AttackType::None,
		ActionPolicy{ 80,  54.f / 30.566f,  Bit(ActionType::Stun) | Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false, false });

	SetPolicy(ActionType::Stun, EntityType::Knight, AttackType::None,
		ActionPolicy{ 85,  96.f / 30.3158f, Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false, false });

	SetPolicy(ActionType::Hit, EntityType::Knight, AttackType::None,
		ActionPolicy{ 90,  50.f / 30.6122f, Bit(ActionType::Dead), false, false, false });

	SetPolicy(ActionType::Dead, EntityType::Knight, AttackType::None,
		ActionPolicy{ 100, 150.f / 30.2013f, 0u, false, false, false });

	
	// FInal Boss
	SetPolicy(ActionType::None, EntityType::Final_Boss, AttackType::None, 
		ActionPolicy{ 0,  0.f, ~0u, false, false, false });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::JumpSlash, 
		ActionPolicy{ 60, 50.f / 30.6122f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false, true });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::MultiSlash, 
		ActionPolicy{ 60, 93.f / 30.3261f, Bit(ActionType::Stun) | Bit(ActionType::Dead), false, false, true });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::DashSlash, 
		ActionPolicy{ 60, 56.f / 30.5455f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false, true });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::Thrust, 
		ActionPolicy{ 60, 56.f / 30.5455f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false, true });

	SetPolicy(ActionType::Attack, EntityType::Final_Boss, AttackType::Slash, 
		ActionPolicy{ 60, 47.f / 30.6522f, Bit(ActionType::Stun) | Bit(ActionType::Dead), true, false, true });

	SetPolicy(ActionType::Stun, EntityType::Final_Boss, AttackType::None, 
		ActionPolicy{ 85, 201.f / 30.15f, Bit(ActionType::Hit) | Bit(ActionType::Dead), false, false, false });

	SetPolicy(ActionType::Hit, EntityType::Final_Boss, AttackType::None,
		ActionPolicy{ 90, 35.f / 30.8824f, Bit(ActionType::Attack) | Bit(ActionType::Dead), false, false, false });

	SetPolicy(ActionType::Dead, EntityType::Final_Boss, AttackType::None, 
		ActionPolicy{ 100, 166.f / 30.1818f, 0u, false, false, false });

}

void ActionManager::LoadProfile()
{
	// TEMP : 추후 데이터 구조 확정 및 툴 완성 후 분리
	ActionProfile attack
	{
		{
			{
				0.0f,
				11.0f / 40.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 11.54f / 100.f,
					.lockDir = true
				},
				YawMode::FaceMoveDir,
				YawParams 
				{
					.turnSpeedRad = 10.0f,
					.yawEpsRad = 0.02f
				}
			},

			{
				11.0f / 40.0f,
				20.0f / 40.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 26.782f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams {}
			},

			{
				20.0f / 40.0f,
				1.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 57.311f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams {}
			},
		}
	};

	SetProfile(ActionType::Attack, EntityType::Knight, AttackType::Light, attack);

	ActionProfile dodge
	{
		{
			{
				0.0f,
				7.0f / 50.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 22.796f / 100.f,
					.lockDir = true
				},
				YawMode::FaceMoveDir,
				YawParams
				{
					.turnSpeedRad = 10.0f,
					.yawEpsRad = 0.02f
				}
			},

			{
				7.0f / 50.0f,
				40.0f / 50.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 319.144f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams {}
			},

			{
				40.0f / 50.0f,
				1.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 6.03f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams {}
			},
		}
	};

	SetProfile(ActionType::Dodge, EntityType::Knight, AttackType::None, dodge);

	ActionProfile thrust
	{
		{
			{
				0.0,
				21.0 / 56.0,
				MoveMode::None,
				MoveParams {},
				YawMode::FaceTarget,
				YawParams
				{
					.turnSpeedRad = 50.0f,
					.yawEpsRad = 0.02f
				}
			},

			{
				21.0 / 56.0,
				31.0 / 56.0,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 98.108f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams{}
			},

			{
				31.0 / 56.0,
				38.0 / 56.0,
				MoveMode::None,
				MoveParams {},
				YawMode::None,
				YawParams{}
			},

			{
				38.0 / 56.0,
				1.0,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 47.39f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams{}
			},
		}
	};

	SetProfile(ActionType::Attack, EntityType::Final_Boss, AttackType::Thrust, thrust);

	ActionProfile slash
	{
		{
			{
				0.0f,
				26.0f / 47.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 8.453f / 100.f,
					.dirMul = -1,
					.lockDir = false
				},
				YawMode::FaceTarget,
				YawParams
				{
					.turnSpeedRad = 10.0f,
					.yawEpsRad = 0.02f
				}
			},

			{
				26.0f / 47.0f,
				37.0f / 47.0f,
				MoveMode::FixedDistance,
				MoveParams
				{
					.distance = 100.467f / 100.f,
					.lockDir = true
				},
				YawMode::None,
				YawParams{}
			},

			{
				37.0f / 47.0f,
				1.0f,
				MoveMode::None,
				MoveParams {},
				YawMode::None,
				YawParams{}
			},
		}
	};

	SetProfile(ActionType::Attack, EntityType::Final_Boss, AttackType::Slash, slash);

	ActionProfile dashSlash
	{
		{
			{
				0.0f,
				20.0f / 56.0f,
				MoveMode::None,
				MoveParams {},
				YawMode::FaceTarget,
				YawParams
				{
					.turnSpeedRad = 150.0f,
					.yawEpsRad = 0.02f
				}
			},

			{
				20.0f / 56.0f,
				25.0f / 56.0f,
				MoveMode::DashToTarget,
				MoveParams
				{
					.maxSpeed = 70.0f,
					.maxTravel = 10000.0f,
					.stopRange = 1.5f,
					.lockDir = true,
				},
				YawMode::None,
				YawParams{}
			},
			
			{
				25.0f / 56.0f,
				1.0f,
				MoveMode::None,
				MoveParams {},
				YawMode::None,
				YawParams{}
			},
		}
	};

	SetProfile(ActionType::Attack, EntityType::Final_Boss, AttackType::DashSlash, dashSlash);

	ActionProfile jumpSlash
	{
		{
			{
				0.0f,
				7.0f / 41.0f,
				MoveMode::None,
				MoveParams {},
				YawMode::FaceTarget,
				YawParams
				{
					.turnSpeedRad = 150.0f,
					.yawEpsRad = 0.01f
				},
				VerticalMode::None,
				VerticalParams {}
			},

			{
				7.0f / 41.0f,
				18.0f / 41.0f,
				MoveMode::DashToTarget,
				MoveParams
				{
					.maxSpeed = 40.0f,
					.maxTravel = 10000.0f,
					.stopRange = 1.5f,
					.lockDir = true,
				},
				YawMode::None,
				YawParams{},
				VerticalMode::FixedDeltaY,
				VerticalParams
				{
					.deltaY = 347.3f / 100.0f
				}
			},

			{
				18.0f / 41.0f,
				26.0f / 41.0f,
				MoveMode::DashToTarget,
				MoveParams
				{
					.maxSpeed = 40.0f,
					.maxTravel = 10000.0f,
					.stopRange = 1.5f,
					.lockDir = true,
				},
				YawMode::None,
				YawParams{},
				VerticalMode::FixedDeltaY,
				VerticalParams
				{
					.deltaY = -347.3f / 100.0f
				}
			},

			{
				26.0f / 41.0f,
				28.0f / 41.0f,
				MoveMode::DashToTarget,
				MoveParams
				{
					.maxSpeed = 70.0f,
					.maxTravel = 10000.0f,
					.stopRange = 1.5f,
					.lockDir = true,
				},
				YawMode::None,
				YawParams{},
				VerticalMode::None,
				VerticalParams {}
			},

			{
				28.0f / 41.0f,
				1.0f,
				MoveMode::None,
				MoveParams {},
				YawMode::None,
				YawParams{},
				VerticalMode::None,
				VerticalParams {}
			},
		}
	};

	SetProfile(ActionType::Attack, EntityType::Final_Boss, AttackType::JumpSlash, jumpSlash);
}

ActionProfile ActionManager::LoadActionProfile(std::string_view path)
{
	return ActionProfile();
}

void ActionManager::SetPolicy(ActionType action, EntityType entity, AttackType attack, const ActionPolicy& policy)
{
	_policyTable[Index(action, entity, attack)] = policy;
}

void ActionManager::SetProfile(ActionType action, EntityType entity, AttackType attack, const ActionProfile& profile)
{
	_profileTable[Index(action, entity, attack)] = profile;
}

const ActionPolicy* ActionManager::FindPolicy(ActionType action, EntityType entity, AttackType attack) const
{
	const auto& policy = _policyTable[Index(action, entity, attack)];
	if (policy.IsValid()) return nullptr;
	return &policy;
}

const ActionProfile* ActionManager::FindProfile(ActionType action, EntityType entity, AttackType attack) const
{
	const auto& profile = _profileTable[Index(action, entity, attack)];
	if (profile.segments.empty()) return nullptr;
	return &profile;
}
