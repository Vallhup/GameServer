#include "pch.h"
#include "AICommandConsumeSystem.h"

#include "AI.h"
#include "Tags.h"
#include "Movement.h"

void AICommandConsumeSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, aiCommand, velocity, locoState] :
		ecs.View<AICommand, Velocity, LocomotionState>(Exclude<DisconnectedTag>()))
	{
        velocity.dir = { 0.0f, 0.0f, 0.0f };
        velocity.isRun = false;

        locoState.isMoving = 
            aiCommand.moveMode == AIMoveMode::Walk ||
            aiCommand.moveMode == AIMoveMode::Run;
        locoState.isRun = aiCommand.moveMode == AIMoveMode::Run;

        if (aiCommand.type != AICommandType::Move) continue;

        switch (aiCommand.moveMode) {
        case AIMoveMode::Walk:
        {
            velocity.dir = aiCommand.moveDir;
            velocity.isRun = false;
            break;
        }
        case AIMoveMode::Run:
        {
            velocity.dir = aiCommand.moveDir;
            velocity.isRun = true;
            break;
        }
        case AIMoveMode::None:
        case AIMoveMode::Hold:
        default:
            break;
        }
	}
}