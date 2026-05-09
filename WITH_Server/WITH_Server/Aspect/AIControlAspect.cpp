#include "pch.h"
#include "AIControlAspect.h"

#include "../AIBehaviorDef.h"
#include "../AIFSMRegistry.h"
#include "../GameDataCatalog.h"
#include "RepComponent.h"
#include "WorldRuntime.h"

#include <algorithm>
#include <utility>

CharacterFeatureFlags AIControlAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::AIControlled;
}

void AIControlAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<AIControlledTag>();
	runtime.RegisterStorage<AITypeComp>();
	runtime.RegisterStorage<AIPerceptionComp>();
	runtime.RegisterStorage<AIBlackboardComp>();
	runtime.RegisterStorage<AIDecisionComp>();
	runtime.RegisterStorage<AIReactionEventQueueComp>();
	runtime.RegisterStorage<AIActionRuntimeComp>();
	runtime.RegisterStorage<AIMovementRuntimeComp>();
	runtime.RegisterStorage<AIIntentFrameComp>();
}

void AIControlAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredAddComponent<AIControlledTag>(entity);

	runtime.DeferredUpsertComponent<AITypeComp>(entity,
		AITypeComp
		{
			.aiType = def.ai->aiType,
			.aiProfileId = def.ai->aiProfileId
		});

	runtime.DeferredAddComponent<AIPerceptionComp>(entity);

	const AIBehaviorProfileDef* profile =
		GameDataCatalog::Current().AIBehaviors().Find(def.ai->aiProfileId);

	runtime.DeferredUpsertComponent<AIBlackboardComp>(entity, 
		BuildAIBlackboardComp(params, profile));

	runtime.DeferredUpsertComponent<AIActionRuntimeComp>(entity,
		std::move(BuildAIActionRuntimeComp(profile)));

	runtime.DeferredAddComponent<AIMovementRuntimeComp>(entity);
	runtime.DeferredAddComponent<AIDecisionComp>(entity);
	runtime.DeferredAddComponent<AIReactionEventQueueComp>(entity);
	runtime.DeferredAddComponent<AIIntentFrameComp>(entity);
}

bool AIControlAspect::Validate(
	const CharacterDef& def,
	std::string& outError) const
{
	if (!def.ai.has_value())
	{
		outError = "AIControlled feature requires CharacterDef.ai";
		return false;
	}
	if (!AIFSMRegistry::IsArchetypeSupported(def.ai->aiType))
	{
		outError =
			"AIControlled archetype has no FSM bundle in AIFSMRegistry";
		return false;
	}
	if (!AIFSMRegistry::IsBehaviorSupported(
		def.ai->aiProfileId))
	{
		outError =
			"AIControlled tuning has no behavior profile in AIFSMRegistry";
		return false;
	}
	return true;
}

AIBlackboardComp AIControlAspect::BuildAIBlackboardComp(
	const AssembleParams& params, 
	const AIBehaviorProfileDef* profile) noexcept
{
	AIBlackboardComp blackboard{};
	blackboard.homePosition = params.position;
	blackboard.hasHomePosition = true;

	if (profile == nullptr)
	{
		blackboard.leashGauge = AIPerceptionTuningDef{}.leashGaugeMax;
	}
	else
	{
		blackboard.leashGauge = profile->perception.leashGaugeMax;
	}

	return blackboard;
}

AIActionRuntimeComp AIControlAspect::BuildAIActionRuntimeComp(
	const AIBehaviorProfileDef* profile) noexcept
{
	AIActionRuntimeComp actionRuntime{};
	if (profile != nullptr)
	{
		actionRuntime.actionCooldownSec.resize(
			profile->combatActionDefs.size() + profile->idleActionDefs.size(), 0.0f);

		AIActionGroupId maxGroupId{ InvalidAIActionGroupId };
		for (const AIActionDef& action : profile->combatActionDefs)
		{
			maxGroupId = std::max(maxGroupId, action.groupId);
		}

		for (const AIActionDef& action : profile->idleActionDefs)
		{
			maxGroupId = std::max(maxGroupId, action.groupId);
		}

		actionRuntime.groupCooldownSec.resize(static_cast<size_t>(maxGroupId), 0.0f);
	}

	return actionRuntime;
}
