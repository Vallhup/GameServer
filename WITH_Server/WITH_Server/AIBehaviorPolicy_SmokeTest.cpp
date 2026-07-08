#include "pch.h"

#include <cassert>
#include <iostream>

#include "AICombatRangePolicy.h"
#include "AIMovementDistanceResolver.h"
#include "AITargetAttackTracker.h"
#include "AITargetCandidateSelector.h"
#include "DataDrivenAICombatActionPolicy.h"
#include "ECS/Components/GameplayContentComponents.h"
#include "IAIState.h"

class AIBehaviorPolicySmokeTest final {
public:
	static void RunAll()
	{
		RunSingleCandidateRepeatFallback();
		RunUnavailableEffectKeepsBasicCandidate();
		RunSelectionCommitBoundary();
		RunCombatRangeHysteresis();
		RunMovementDistanceHysteresis();
		RunAttackCountRetargetRequest();
		RunAlternatePlayerTargetSelection();
		RunForcedTargetSelection();

		std::cout << "[PASS] AIBehaviorPolicy smoke\n";
	}

private:
	struct CombatFixture
	{
		std::array<AIActionDef, 2> actions{};
		std::array<AIMovementProfileDef, 1> movementProfiles{};
		AIBehaviorProfileDef profile{};
		AIPerceptionComp perception{};
		AbilityStateComp abilityState{};
		AIActionRuntimeComp runtime{};
		AIContext context{};

		explicit CombatFixture(size_t actionCount)
		{
			movementProfiles[0].veryCloseDistance = 2.0;
			movementProfiles[0].closeDistance = 5.0;
			movementProfiles[0].midDistance = 8.0;
			movementProfiles[0].farDistance = 12.0;
			movementProfiles[0].preferredMinDistance = 3.0;
			movementProfiles[0].preferredMaxDistance = 6.0;

			profile.combatActionDefs =
				std::span<const AIActionDef>(actions.data(), actionCount);
			profile.movementProfiles = movementProfiles;

			perception.hasTarget = true;
			perception.targetVisible = true;
			perception.targetInFront = true;
			perception.distanceToTarget = 4.0;
			perception.selectedTarget = Entity{ 2, 1 };

			runtime.actionCooldownSec.resize(actionCount, 0.0f);

			context.self = Entity{ 1, 1 };
			context.perception = &perception;
			context.abilityState = &abilityState;
			context.actionRuntime = &runtime;
			context.behaviorProfile = &profile;
		}
	};

	static AIActionDef MakeAction(
		AbilityId abilityId,
		AIActionRole role = AIActionRole::Basic)
	{
		AIActionDef action{};
		action.abilityId = abilityId;
		action.weight = 100;
		action.actionRole = role;
		action.condition.distanceBucket = AIActionDistanceBucket::Any;
		action.condition.requiresAbilityAvailable = false;
		action.aiCooldownSec = 2.0f;
		action.globalCooldownSec = 1.0f;
		return action;
	}

	static void RunSingleCandidateRepeatFallback()
	{
		CombatFixture fixture{ 1 };
		fixture.actions[0] = MakeAction(static_cast<AbilityId>(101));
		fixture.actions[0].forbidImmediateRepeat = true;
		fixture.runtime.lastUsedAbilityId = fixture.actions[0].abilityId;

		DataDrivenAICombatActionPolicy policy;
		const CombatActionSelection selection =
			policy.SelectAction(fixture.context);

		assert(selection.shouldAttack);
		assert(selection.selectedActionIndex == 0);
		assert(selection.selectedAbilityId == fixture.actions[0].abilityId);
	}

	static void RunUnavailableEffectKeepsBasicCandidate()
	{
		CombatFixture fixture{ 2 };
		fixture.actions[0] = MakeAction(static_cast<AbilityId>(201));
		fixture.actions[1] = MakeAction(
			static_cast<AbilityId>(202),
			AIActionRole::Effect);
		fixture.actions[1].requiresBasicActionCount = 2;
		fixture.actions[1].resetsBasicActionCount = true;

		fixture.runtime.basicActionCountSinceEffect = 2;
		fixture.runtime.actionCooldownSec[1] = 3.0f;

		DataDrivenAICombatActionPolicy policy;
		const CombatActionSelection selection =
			policy.SelectAction(fixture.context);

		assert(selection.shouldAttack);
		assert(selection.selectedActionIndex == 0);
		assert(selection.selectedAbilityId == fixture.actions[0].abilityId);
	}

	static void RunSelectionCommitBoundary()
	{
		CombatFixture fixture{ 1 };
		fixture.actions[0] = MakeAction(static_cast<AbilityId>(301));

		DataDrivenAICombatActionPolicy policy;
		const CombatActionSelection selection =
			policy.SelectAction(fixture.context);

		assert(selection.shouldAttack);
		assert(fixture.runtime.actionSequence == 0);
		assert(fixture.runtime.lastUsedAbilityId == InvalidAbilityId);
		assert(fixture.runtime.actionCooldownSec[0] == 0.0f);

		policy.CommitSelection(fixture.context, selection);

		assert(fixture.runtime.actionSequence == 1);
		assert(fixture.runtime.lastUsedAbilityId ==
			fixture.actions[0].abilityId);
		assert(fixture.runtime.actionCooldownSec[0] ==
			fixture.actions[0].aiCooldownSec);
	}

	static void RunCombatRangeHysteresis()
	{
		CombatFixture fixture{ 1 };
		fixture.actions[0] = MakeAction(static_cast<AbilityId>(401));

		AIPerceptionTuningDef tuning{};
		tuning.attackRange = 12.0;
		tuning.combatExitRangeBonus = 0.5;

		fixture.context.perceptionTuning = &tuning;
		fixture.perception.distanceToTarget = 12.2;
		fixture.perception.targetInFront = true;

		assert(!AICombatRangePolicy::ShouldEnterCombat(fixture.context));
		assert(!AICombatRangePolicy::ShouldExitCombat(fixture.context));

		fixture.perception.distanceToTarget = 11.5;
		assert(AICombatRangePolicy::ShouldEnterCombat(fixture.context));

		fixture.runtime.globalActionCooldownSec = 1.0f;
		assert(!AICombatRangePolicy::ShouldEnterCombat(fixture.context));
		fixture.runtime.globalActionCooldownSec = 0.0f;

		fixture.perception.distanceToTarget = 12.6;
		assert(AICombatRangePolicy::ShouldExitCombat(fixture.context));

		fixture.perception.distanceToTarget = 11.5;
		fixture.perception.targetInFront = false;
		assert(AICombatRangePolicy::ShouldHoldChasePosition(fixture.context));
		assert(!AICombatRangePolicy::ShouldEnterCombat(fixture.context));
	}

	static void RunMovementDistanceHysteresis()
	{
		AIMovementProfileDef profile{};
		profile.veryCloseDistance = 2.0;
		profile.closeDistance = 4.2;
		profile.midDistance = 8.0;
		profile.farDistance = 16.0;
		profile.preferredMinDistance = 3.2;
		profile.preferredMaxDistance = 7.0;
		profile.distanceHysteresis = 0.3;
		profile.closeBehavior = AIMovementBehavior::Approach;
		profile.preferredBehavior = AIMovementBehavior::Hold;

		AIMovementRuntimeComp runtime{};
		assert(AIMovementDistanceResolver::ResolveBehavior(
			profile,
			3.1,
			&runtime) == AIMovementBehavior::Approach);

		assert(AIMovementDistanceResolver::ResolveBehavior(
			profile,
			3.25,
			&runtime) == AIMovementBehavior::Approach);

		assert(AIMovementDistanceResolver::ResolveBehavior(
			profile,
			3.55,
			&runtime) == AIMovementBehavior::Hold);
	}

	static void RunAttackCountRetargetRequest()
	{
		AIBehaviorProfileDef profile{};
		profile.targeting.attacksBeforeForcedRetarget = 3;

		AIBlackboardComp blackboard{};
		AIContext context{};
		context.behaviorProfile = &profile;
		context.blackboard = &blackboard;

		const Entity target{ 10, 1 };
		AITargetAttackTracker::RecordCommittedAttack(context, target);
		AITargetAttackTracker::RecordCommittedAttack(context, target);
		assert(!blackboard.alternateTargetRetargetRequested);
		assert(blackboard.committedAttackCount == 2);

		AITargetAttackTracker::RecordCommittedAttack(context, target);
		assert(blackboard.alternateTargetRetargetRequested);
		assert(blackboard.committedAttackCount == 0);
	}

	static void RunAlternatePlayerTargetSelection()
	{
		const AITargetCandidate current{
			.entity = Entity{ 20, 1 },
			.isCurrentTarget = true,
			.score = 20.0
		};
		const AITargetCandidate alternate{
			.entity = Entity{ 21, 1 },
			.score = 1.0
		};

		AITargetSelectionSnapshot snapshot{
			.best = current,
			.current = current,
			.alternatePlayer = alternate,
			.foundAny = true,
			.hasCurrent = true,
			.hasAlternatePlayer = true,
			.alternatePlayerRetargetRequested = true,
			.switchScoreMargin = 10.0
		};

		assert(AITargetCandidateSelector::Select(snapshot).entity ==
			alternate.entity);

		snapshot.hasAlternatePlayer = false;
		assert(AITargetCandidateSelector::Select(snapshot).entity ==
			current.entity);
	}

	static void RunForcedTargetSelection()
	{
		const AITargetCandidate best{
			.entity = Entity{ 30, 1 },
			.score = 10.0
		};
		const AITargetCandidate forced{
			.entity = Entity{ 31, 1 },
			.score = 1.0
		};

		const AITargetSelectionSnapshot snapshot{
			.best = best,
			.forced = forced,
			.foundAny = true,
			.hasForced = true,
			.forceRetarget = true
		};

		assert(AITargetCandidateSelector::Select(snapshot).entity ==
			forced.entity);
	}
};

void RunAIBehaviorPolicySmokeTests()
{
	AIBehaviorPolicySmokeTest::RunAll();
}
