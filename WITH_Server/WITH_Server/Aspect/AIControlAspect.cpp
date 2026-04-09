#include "pch.h"
#include "AIControlAspect.h"

#include "../AIFSMRegistry.h"
#include "../ECS/GameplayRuntimeComponents.h"
#include "RepComponent.h"
#include "WorldRuntime.h"

CharacterFeatureFlags AIControlAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::AIControlled;
}

void AIControlAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<AIControlledTag>();
	runtime.RegisterStorage<AITypeComp>();
	runtime.RegisterStorage<AIPerceptionComp>();
	runtime.RegisterStorage<AIPerceptionTuningComp>();
	runtime.RegisterStorage<AIBlackboardComp>();
	runtime.RegisterStorage<AIDecisionComp>();
	runtime.RegisterStorage<AIDecisionTuningComp>();
	runtime.RegisterStorage<AIReactionComp>();
	runtime.RegisterStorage<AICommandFrameComp>();
}

void AIControlAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	(void)params;
	runtime.DeferredAddComponent<AIControlledTag>(entity);
	runtime.DeferredUpsertComponent<AITypeComp>(
		entity,
		AITypeComp{ .aiType = def.ai->aiType });
	runtime.DeferredAddComponent<AIPerceptionComp>(entity);
	runtime.DeferredAddComponent<AIPerceptionTuningComp>(entity);
	runtime.DeferredAddComponent<AIBlackboardComp>(entity);
	runtime.DeferredAddComponent<AIDecisionComp>(entity);
	runtime.DeferredAddComponent<AIDecisionTuningComp>(entity);
	runtime.DeferredAddComponent<AIReactionComp>(entity);
	runtime.DeferredAddComponent<AICommandFrameComp>(entity);
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
	return true;
}
