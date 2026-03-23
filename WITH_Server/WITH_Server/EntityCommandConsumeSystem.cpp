#include "pch.h"
#include "EntityCommandConsumeSystem.h"

void EntityCommandConsumeSystem::Execute(const double dT)
{
	for(const auto& [entity, command, velocity, locomotion, actionIntent] :
		_runtime.GetECS().View<EntityCommandFrame, Velocity, LocomotionState, 
		ActionIntent>(Exclude<DisconnectedTag>()))
	{
		ConsumeMove(entity, command, velocity, locomotion);
		ConsumeLook(entity, command);
		ConsumeGuard(entity, command, actionIntent);
		ConsumeAction(entity, command);
	}

}

void EntityCommandConsumeSystem::ConsumeMove(
	Entity entity, 
	const EntityCommandFrame& command, 
	Velocity& velocity, 
	LocomotionState& locomotion) const
{
	if (!command.move.hasMove)
	{
		locomotion.isMoving = false;
		locomotion.isRun = false;
		velocity.dir = { 0, 0, 0 };
		return;
	}

	locomotion.isMoving = true;
	locomotion.isRun = command.move.moveRun;
	velocity.dir = command.move.moveDir;
}

void EntityCommandConsumeSystem::ConsumeLook(
	Entity entity, 
	const EntityCommandFrame& command) const
{

}

void EntityCommandConsumeSystem::ConsumeGuard(
	Entity entity, 
	const EntityCommandFrame& command, 
	ActionIntent& actionIntent) const
{
	if (command.guard.hasGuard)
		actionIntent.guard = command.guard.guardHeld;
}

void EntityCommandConsumeSystem::ConsumeAction(
	Entity entity, 
	const EntityCommandFrame& command) const
{
	if (!command.action.hasAction) return;

	ActionRequestReason reason{ ActionRequestReason::None };

	switch (command.source) {
	case CommandSource::Player: reason = ActionRequestReason::FromInput; break;
	case CommandSource::AI:		reason = ActionRequestReason::FromAI;	 break;
	default:					reason = ActionRequestReason::None;		 break;
	}

	ActionRequestEvent ev;
	ev.entity = entity;
	ev.actionType = command.action.actionType;
	ev.attackType = command.action.attackType;
	ev.reason = reason;

	_runtime.Events().Queue<ActionRequestEvent>().Publish(ev);
}