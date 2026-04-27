#pragma once

#include "AIBehaviorDef.h"
#include "ActionDef.h"
#include "BodyCollisionTypes.h"
#include "BuffDef.h"
#include "CharacterDef.h"
#include "SpawnSetDef.h"

#include <string_view>

bool ParseDefString(std::string_view text, ActionId& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionKind& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionCancelKind& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionCandidateSelectionPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionCombatApplyTo& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionEndType& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionInterruptCauseType& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionNormalizedPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionRequestRequirementType& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionRequestSemantic& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionResourceConsumeTiming& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionResourceType& outValue) noexcept;
bool ParseDefString(std::string_view text, ActionWindowPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, AIArchetype& outValue) noexcept;
bool ParseDefString(std::string_view text, AIMovementPolicyKind& outValue) noexcept;
bool ParseDefString(std::string_view text, AICombatActionPolicyKind& outValue) noexcept;
bool ParseDefString(std::string_view text, AIIdleActionPolicyKind& outValue) noexcept;
bool ParseDefString(std::string_view text, AIReactionPolicyKind& outValue) noexcept;
bool ParseDefString(std::string_view text, BodyPushability& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffApplyRequirementType& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffCounterEventType& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffEffectType& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffFamilyId& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffFamilyStackPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffId& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffKind& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffModifierConditionType& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffModifierTiming& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffRemoveRuleType& outValue) noexcept;
bool ParseDefString(std::string_view text, BuffTier& outValue) noexcept;
bool ParseDefString(std::string_view text, CharacterFeatureFlags& outValue) noexcept;
bool ParseDefString(std::string_view text, CharacterId& outValue) noexcept;
bool ParseDefString(std::string_view text, CharacterRole& outValue) noexcept;
bool ParseDefString(std::string_view text, CombatEffectType& outValue) noexcept;
bool ParseDefString(std::string_view text, CombatReferenceFrame& outValue) noexcept;
bool ParseDefString(std::string_view text, CombatWindowType& outValue) noexcept;
bool ParseDefString(std::string_view text, DirectionPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, DirectionSampleTiming& outValue) noexcept;
bool ParseDefString(std::string_view text, DurationPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, EventType& outValue) noexcept;
bool ParseDefString(std::string_view text, Faction& outValue) noexcept;
bool ParseDefString(std::string_view text, GameplayStateFlag& outValue) noexcept;
bool ParseDefString(std::string_view text, HorizontalMovementMode& outValue) noexcept;
bool ParseDefString(std::string_view text, LocomotionMode& outValue) noexcept;
bool ParseDefString(std::string_view text, OverlappingPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, ReapplyPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, RespawnPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, RotationMode& outValue) noexcept;
bool ParseDefString(std::string_view text, SpawnConditionType& outValue) noexcept;
bool ParseDefString(std::string_view text, SpawnSetId& outValue) noexcept;
bool ParseDefString(std::string_view text, StatType& outValue) noexcept;
bool ParseDefString(std::string_view text, TriggerConditionType& outValue) noexcept;
bool ParseDefString(std::string_view text, VerticalMovementMode& outValue) noexcept;

bool ParseAITuningIdString(std::string_view text, AITuningId& outValue) noexcept;
bool ParseCharacterActionProfileIdString(
	std::string_view text,
	CharacterActionProfileId& outValue) noexcept;
bool ParseActionFallbackReactionProfileIdString(
	std::string_view text,
	ActionFallbackReactionProfileId& outValue) noexcept;
bool ParseActionInputBindingProfileIdString(
	std::string_view text,
	ActionInputBindingProfileId& outValue) noexcept;
bool ParseAnimationBindingProfileIdString(
	std::string_view text,
	AnimationBindingProfileId& outValue) noexcept;
bool ParseAnimationIdString(std::string_view text, AnimationId& outValue) noexcept;
bool ParseSpawnPointIdString(std::string_view text, SpawnPointId& outValue) noexcept;
