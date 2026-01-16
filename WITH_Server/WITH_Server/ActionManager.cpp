#include "pch.h"
#include "ActionManager.h"

ActionManager::ActionManager()
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

const ActionProfile* ActionManager::GetActionMoveProfile(ActionType actionType) const
{
	auto it = _actionProfiles.find(actionType);
	if (it != _actionProfiles.end()) return it->second.get();
	return nullptr;
}

ActionProfile ActionManager::LoadActionProfile(std::string_view path)
{
	return ActionProfile();
}
