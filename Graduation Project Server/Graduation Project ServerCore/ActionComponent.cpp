#include "pch.h"
#include "ActionComponent.h"

void ActionComponent::LogicUpdate(float deltaTime)
{
	if (_currentAction) {
		_currentAction->Update(deltaTime);
		if (_currentAction->IsFinished()) {
			_currentAction->End();
			_currentAction.reset();
		}
	}
}

void ActionComponent::StartAttack()
{
	if (not _currentAction) {
		_currentAction = std::make_unique<AttackAction>(_owner);
	}
}

void ActionComponent::StartDodge()
{
	if (not _currentAction) {
		_currentAction = std::make_unique<DodgeAction>(_owner);
	}
}
