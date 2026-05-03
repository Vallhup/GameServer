#include "pch.h"
#include "DefEnumString.h"

#include <unordered_map>

bool ParseDefString(std::string_view text, ActionId& outValue) noexcept
{
	static const std::unordered_map<std::string_view, ActionId> kValues =
	{
		{ "None", ActionId::None },
		{ "Knight_LightAttack1", ActionId::Knight_LightAttack1 },
		{ "Knight_LightAttack2", ActionId::Knight_LightAttack2 },
		{ "Knight_LightAttack3", ActionId::Knight_LightAttack3 },
		{ "Knight_HeavyAttack", ActionId::Knight_HeavyAttack },
		{ "Knight_SpecialAttack", ActionId::Knight_SpecialAttack },
		{ "Knight_Dodge", ActionId::Knight_Dodge },
		{ "Knight_Parry", ActionId::Knight_Parry },
		{ "Knight_Stun", ActionId::Knight_Stun },
		{ "Knight_Hit", ActionId::Knight_Hit },
		{ "Knight_Guard", ActionId::Knight_Guard },
		{ "Knight_UseHpPotion", ActionId::Knight_UseHpPotion },
		{ "Knight_Dead", ActionId::Knight_Dead },
		{ "Lancer_LightAttack1", ActionId::Lancer_LightAttack1 },
		{ "Lancer_LightAttack2", ActionId::Lancer_LightAttack2 },
		{ "Lancer_LightAttack3", ActionId::Lancer_LightAttack3 },
		{ "Lancer_HeavyAttack", ActionId::Lancer_HeavyAttack },
		{ "Lancer_SpecialAttack", ActionId::Lancer_SpecialAttack },
		{ "Lancer_Dodge", ActionId::Lancer_Dodge },
		{ "Lancer_Parry", ActionId::Lancer_Parry },
		{ "Lancer_Stun", ActionId::Lancer_Stun },
		{ "Lancer_Hit", ActionId::Lancer_Hit },
		{ "Lancer_Guard", ActionId::Lancer_Guard },
		{ "Lancer_UseHpPotion", ActionId::Lancer_UseHpPotion },
		{ "Lancer_Dead", ActionId::Lancer_Dead },
		{ "Paladin_LightAttack1", ActionId::Paladin_LightAttack1 },
		{ "Paladin_LightAttack2", ActionId::Paladin_LightAttack2 },
		{ "Paladin_LightAttack3", ActionId::Paladin_LightAttack3 },
		{ "Paladin_HeavyAttack", ActionId::Paladin_HeavyAttack },
		{ "Paladin_SpecialAttack", ActionId::Paladin_SpecialAttack },
		{ "Paladin_Dodge", ActionId::Paladin_Dodge },
		{ "Paladin_Parry", ActionId::Paladin_Parry },
		{ "Paladin_Stun", ActionId::Paladin_Stun },
		{ "Paladin_Hit", ActionId::Paladin_Hit },
		{ "Paladin_Guard", ActionId::Paladin_Guard },
		{ "Paladin_UseHpPotion", ActionId::Paladin_UseHpPotion },
		{ "Paladin_Dead", ActionId::Paladin_Dead },
		{ "Imp_melee1", ActionId::Imp_melee1 },
		{ "Imp_melee2", ActionId::Imp_melee2 },
		{ "Imp_melee3", ActionId::Imp_melee3 },
		{ "Imp_melee4", ActionId::Imp_melee4 },
		{ "Imp_melee5", ActionId::Imp_melee5 },
		{ "Imp_Jump", ActionId::Imp_Jump },
		{ "Imp_Stun", ActionId::Imp_Stun },
		{ "Imp_Hit", ActionId::Imp_Hit },
		{ "Imp_Dead", ActionId::Imp_Dead },
		{ "DemonStriker_Melee_1", ActionId::DemonStriker_Melee_1 },
		{ "DemonStriker_Melee_2", ActionId::DemonStriker_Melee_2 },
		{ "DemonStriker_Melee_3", ActionId::DemonStriker_Melee_3 },
		{ "DemonStriker_Melee_4", ActionId::DemonStriker_Melee_4 },
		{ "DemonStriker_Gun_Shoot_1", ActionId::DemonStriker_Gun_Shoot_1 },
		{ "DemonStriker_Gun_Shoot_2", ActionId::DemonStriker_Gun_Shoot_2 },
		{ "DemonStriker_Gun_Shoot_3", ActionId::DemonStriker_Gun_Shoot_3 },
		{ "DemonStriker_Gun_Shoot_4", ActionId::DemonStriker_Gun_Shoot_4 },
		{ "DemonStriker_Jump_1", ActionId::DemonStriker_Jump_1 },
		{ "DemonStriker_Jump_2", ActionId::DemonStriker_Jump_2 },
		{ "DemonStriker_Stun", ActionId::DemonStriker_Stun },
		{ "DemonStriker_Hit", ActionId::DemonStriker_Hit },
		{ "DemonStriker_Dead", ActionId::DemonStriker_Dead },
		{ "DemonExecutioner_Melee_1", ActionId::DemonExecutioner_Melee_1 },
		{ "DemonExecutioner_Melee_2", ActionId::DemonExecutioner_Melee_2 },
		{ "DemonExecutioner_Melee_3", ActionId::DemonExecutioner_Melee_3 },
		{ "DemonExecutioner_Melee_4", ActionId::DemonExecutioner_Melee_4 },
		{ "DemonExecutioner_Melee_5", ActionId::DemonExecutioner_Melee_5 },
		{ "DemonExecutioner_Melee_6", ActionId::DemonExecutioner_Melee_6 },
		{ "DemonExecutioner_Jump_1", ActionId::DemonExecutioner_Jump_1 },
		{ "DemonExecutioner_Jump_2", ActionId::DemonExecutioner_Jump_2 },
		{ "DemonExecutioner_Stun", ActionId::DemonExecutioner_Stun },
		{ "DemonExecutioner_Hit", ActionId::DemonExecutioner_Hit },
		{ "DemonExecutioner_Dead", ActionId::DemonExecutioner_Dead },
		{ "BigDemonWarrior_Melee_1", ActionId::BigDemonWarrior_Melee_1 },
		{ "BigDemonWarrior_Melee_2", ActionId::BigDemonWarrior_Melee_2 },
		{ "BigDemonWarrior_Melee_3", ActionId::BigDemonWarrior_Melee_3 },
		{ "BigDemonWarrior_Melee_4", ActionId::BigDemonWarrior_Melee_4 },
		{ "BigDemonWarrior_Melee_5", ActionId::BigDemonWarrior_Melee_5 },
		{ "BigDemonWarrior_Melee_6", ActionId::BigDemonWarrior_Melee_6 },
		{ "BigDemonWarrior_Melee_7", ActionId::BigDemonWarrior_Melee_7 },
		{ "BigDemonWarrior_Melee_8", ActionId::BigDemonWarrior_Melee_8 },
		{ "BigDemonWarrior_Jump", ActionId::BigDemonWarrior_Jump },
		{ "BigDemonWarrior_BattleCry", ActionId::BigDemonWarrior_BattleCry },
		{ "BigDemonWarrior_Stun", ActionId::BigDemonWarrior_Stun },
		{ "BigDemonWarrior_Hit", ActionId::BigDemonWarrior_Hit },
		{ "BigDemonWarrior_Dead", ActionId::BigDemonWarrior_Dead },
		{ "FinalBoss_Thrust", ActionId::FinalBoss_Thrust },
		{ "FinalBoss_Slash", ActionId::FinalBoss_Slash },
		{ "FinalBoss_DashSlash", ActionId::FinalBoss_DashSlash },
		{ "FinalBoss_JumpSlash", ActionId::FinalBoss_JumpSlash },
		{ "FinalBoss_MultiSlash", ActionId::FinalBoss_MultiSlash },
		{ "FinalBoss_Meteor", ActionId::FinalBoss_Meteor },
		{ "FinalBoss_Gimmic1", ActionId::FinalBoss_Gimmic1 },
		{ "FinalBoss_Gimmic2", ActionId::FinalBoss_Gimmic2 },
		{ "FinalBoss_Stun", ActionId::FinalBoss_Stun },
		{ "FinalBoss_Hit", ActionId::FinalBoss_Hit },
		{ "FinalBoss_Dead", ActionId::FinalBoss_Dead }
	};

	const auto it = kValues.find(text);
	if (it == kValues.end())
		return false;

	outValue = it->second;
	return true;
}

bool ParseDefString(std::string_view text, ActionKind& outValue) noexcept
{
	if (text == "None") { outValue = ActionKind::None; return true; }
	if (text == "Attack") { outValue = ActionKind::Attack; return true; }
	if (text == "Dodge") { outValue = ActionKind::Dodge; return true; }
	if (text == "Parry") { outValue = ActionKind::Parry; return true; }
	if (text == "Stun") { outValue = ActionKind::Stun; return true; }
	if (text == "Hit") { outValue = ActionKind::Hit; return true; }
	if (text == "Guard") { outValue = ActionKind::Guard; return true; }
	if (text == "UseItem") { outValue = ActionKind::UseItem; return true; }
	if (text == "NonCombat") { outValue = ActionKind::NonCombat; return true; }
	if (text == "Dead") { outValue = ActionKind::Dead; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionCancelKind& outValue) noexcept
{
	if (text == "Combo") { outValue = ActionCancelKind::Combo; return true; }
	if (text == "HoldRelease") { outValue = ActionCancelKind::HoldRelease; return true; }
	if (text == "LightAttackCancel") { outValue = ActionCancelKind::LightAttackCancel; return true; }
	if (text == "HeavyAttackCancel") { outValue = ActionCancelKind::HeavyAttackCancel; return true; }
	if (text == "DodgeCancel") { outValue = ActionCancelKind::DodgeCancel; return true; }
	if (text == "ParryCancel") { outValue = ActionCancelKind::ParryCancel; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	ActionCandidateSelectionPolicy& outValue) noexcept
{
	if (text == "OrderedFirstValid") { outValue = ActionCandidateSelectionPolicy::OrderedFirstValid; return true; }
	if (text == "HighestPriorityValid") { outValue = ActionCandidateSelectionPolicy::HighestPriorityValid; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionCombatApplyTo& outValue) noexcept
{
	if (text == "FrontPhysical") { outValue = ActionCombatApplyTo::FrontPhysical; return true; }
	if (text == "ParryableAttack") { outValue = ActionCombatApplyTo::ParryableAttack; return true; }
	if (text == "GuardableAttack") { outValue = ActionCombatApplyTo::GuardableAttack; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionEndType& outValue) noexcept
{
	if (text == "NaturalEnd") { outValue = ActionEndType::NaturalEnd; return true; }
	if (text == "HoldRelease") { outValue = ActionEndType::HoldRelease; return true; }
	if (text == "ImmediateTransition") { outValue = ActionEndType::ImmediateTransition; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	ActionInterruptCauseType& outValue) noexcept
{
	if (text == "OnHitReceived") { outValue = ActionInterruptCauseType::OnHitReceived; return true; }
	if (text == "OnParried") { outValue = ActionInterruptCauseType::OnParried; return true; }
	if (text == "OnHpZero") { outValue = ActionInterruptCauseType::OnHpZero; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionNormalizedPolicy& outValue) noexcept
{
	if (text == "FixedDuration") { outValue = ActionNormalizedPolicy::FixedDuration; return true; }
	if (text == "Holdable") { outValue = ActionNormalizedPolicy::Holdable; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	ActionRequestRequirementType& outValue) noexcept
{
	if (text == "None") { outValue = ActionRequestRequirementType::None; return true; }
	if (text == "HasEnoughStamina") { outValue = ActionRequestRequirementType::HasEnoughStamina; return true; }
	if (text == "MissingStateFlag") { outValue = ActionRequestRequirementType::MissingStateFlag; return true; }
	if (text == "HasStateFlag") { outValue = ActionRequestRequirementType::HasStateFlag; return true; }
	if (text == "HasTarget") { outValue = ActionRequestRequirementType::HasTarget; return true; }
	if (text == "IsGrounded") { outValue = ActionRequestRequirementType::IsGrounded; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionRequestSemantic& outValue) noexcept
{
	if (text == "None") { outValue = ActionRequestSemantic::None; return true; }
	if (text == "LightAttack") { outValue = ActionRequestSemantic::LightAttack; return true; }
	if (text == "HeavyAttack") { outValue = ActionRequestSemantic::HeavyAttack; return true; }
	if (text == "Dodge") { outValue = ActionRequestSemantic::Dodge; return true; }
	if (text == "Parry") { outValue = ActionRequestSemantic::Parry; return true; }
	if (text == "GuardStart") { outValue = ActionRequestSemantic::GuardStart; return true; }
	if (text == "UseItem") { outValue = ActionRequestSemantic::UseItem; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	ActionResourceConsumeTiming& outValue) noexcept
{
	if (text == "OnRequest") { outValue = ActionResourceConsumeTiming::OnRequest; return true; }
	if (text == "OnCommit") { outValue = ActionResourceConsumeTiming::OnCommit; return true; }
	if (text == "OnWindowEnter") { outValue = ActionResourceConsumeTiming::OnWindowEnter; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionResourceType& outValue) noexcept
{
	if (text == "Hp") { outValue = ActionResourceType::Hp; return true; }
	if (text == "Stamina") { outValue = ActionResourceType::Stamina; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ActionWindowPolicy& outValue) noexcept
{
	if (text == "Always") { outValue = ActionWindowPolicy::Always; return true; }
	if (text == "Range") { outValue = ActionWindowPolicy::Range; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIArchetype& outValue) noexcept
{
	if (text == "None") { outValue = AIArchetype::None; return true; }
	if (text == "Humanoid") { outValue = AIArchetype::Humanoid; return true; }
	if (text == "NormalMonster") { outValue = AIArchetype::NormalMonster; return true; }
	if (text == "FirstBossMonster") { outValue = AIArchetype::FirstBossMonster; return true; }
	if (text == "MidBossMonster") { outValue = AIArchetype::MidBossMonster; return true; }
	if (text == "FinalBossMonster") { outValue = AIArchetype::FinalBossMonster; return true; }
	return false;
}

bool ParseAITuningIdString(std::string_view text, AITuningId& outValue) noexcept
{
	if (text == "None") { outValue = AITuningIds::None; return true; }
	if (text == "Imp") { outValue = AITuningIds::Imp; return true; }
	if (text == "DemonStriker") { outValue = AITuningIds::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = AITuningIds::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = AITuningIds::BigDemonWarrior; return true; }
	if (text == "FinalBoss") { outValue = AITuningIds::FinalBoss; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIMovementPolicyKind& outValue) noexcept
{
	if (text == "None") { outValue = AIMovementPolicyKind::None; return true; }
	if (text == "Normal") { outValue = AIMovementPolicyKind::Normal; return true; }
	if (text == "BossPattern") { outValue = AIMovementPolicyKind::BossPattern; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AICombatActionPolicyKind& outValue) noexcept
{
	if (text == "None") { outValue = AICombatActionPolicyKind::None; return true; }
	if (text == "Imp") { outValue = AICombatActionPolicyKind::Imp; return true; }
	if (text == "Weighted") { outValue = AICombatActionPolicyKind::Weighted; return true; }
	if (text == "BossPattern") { outValue = AICombatActionPolicyKind::BossPattern; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIIdleActionPolicyKind& outValue) noexcept
{
	if (text == "None") { outValue = AIIdleActionPolicyKind::None; return true; }
	if (text == "Weighted") { outValue = AIIdleActionPolicyKind::Weighted; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIReactionPolicyKind& outValue) noexcept
{
	if (text == "None") { outValue = AIReactionPolicyKind::None; return true; }
	if (text == "Normal") { outValue = AIReactionPolicyKind::Normal; return true; }
	if (text == "BossPattern") { outValue = AIReactionPolicyKind::BossPattern; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BodyPushability& outValue) noexcept
{
	if (text == "None") { outValue = BodyPushability::None; return true; }
	if (text == "Kinematic") { outValue = BodyPushability::Kinematic; return true; }
	if (text == "Dynamic") { outValue = BodyPushability::Dynamic; return true; }
	return false;
}

bool ParseCharacterActionProfileIdString(
	std::string_view text,
	CharacterActionProfileId& outValue) noexcept
{
	if (text == "Knight") { outValue = CharacterActionProfileIds::Knight; return true; }
	if (text == "Imp") { outValue = CharacterActionProfileIds::Imp; return true; }
	if (text == "FinalBoss") { outValue = CharacterActionProfileIds::FinalBoss; return true; }
	if (text == "DemonStriker") { outValue = CharacterActionProfileIds::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = CharacterActionProfileIds::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = CharacterActionProfileIds::BigDemonWarrior; return true; }
	return false;
}

bool ParseActionFallbackReactionProfileIdString(
	std::string_view text,
	ActionFallbackReactionProfileId& outValue) noexcept
{
	if (text == "Knight") { outValue = ActionFallbackReactionProfileIds::Knight; return true; }
	if (text == "Imp") { outValue = ActionFallbackReactionProfileIds::Imp; return true; }
	if (text == "FinalBoss") { outValue = ActionFallbackReactionProfileIds::FinalBoss; return true; }
	if (text == "DemonStriker") { outValue = ActionFallbackReactionProfileIds::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = ActionFallbackReactionProfileIds::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = ActionFallbackReactionProfileIds::BigDemonWarrior; return true; }
	return false;
}

bool ParseActionInputBindingProfileIdString(
	std::string_view text,
	ActionInputBindingProfileId& outValue) noexcept
{
	if (text == "Knight") { outValue = ActionInputBindingProfileIds::Knight; return true; }
	if (text == "Imp") { outValue = ActionInputBindingProfileIds::Imp; return true; }
	if (text == "FinalBoss") { outValue = ActionInputBindingProfileIds::FinalBoss; return true; }
	if (text == "DemonStriker") { outValue = ActionInputBindingProfileIds::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = ActionInputBindingProfileIds::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = ActionInputBindingProfileIds::BigDemonWarrior; return true; }
	return false;
}

bool ParseAnimationBindingProfileIdString(
	std::string_view text,
	AnimationBindingProfileId& outValue) noexcept
{
	if (text == "Knight") { outValue = AnimationBindingProfileIds::Knight; return true; }
	if (text == "Imp") { outValue = AnimationBindingProfileIds::Imp; return true; }
	if (text == "FinalBoss") { outValue = AnimationBindingProfileIds::FinalBoss; return true; }
	if (text == "DemonStriker") { outValue = AnimationBindingProfileIds::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = AnimationBindingProfileIds::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = AnimationBindingProfileIds::BigDemonWarrior; return true; }
	return false;
}

bool ParseAnimationIdString(std::string_view text, AnimationId& outValue) noexcept
{
	static const std::unordered_map<std::string_view, AnimationId> kValues =
	{
		{ "None", AnimationId::None },
		{ "Knight_Idle", AnimationId::Knight_Idle },
		{ "Knight_Walk", AnimationId::Knight_Walk },
		{ "Knight_Run", AnimationId::Knight_Run },
		{ "Knight_LightAttack1", AnimationId::Knight_AttackCombo1 },
		{ "Knight_Dodge", AnimationId::Knight_Dodge },
		{ "Knight_Parry", AnimationId::Knight_Parry },
		{ "Knight_Stun", AnimationId::Knight_Stun },
		{ "Knight_Hit", AnimationId::Knight_Hit },
		{ "Knight_Guard", AnimationId::Knight_Guard },
		{ "Knight_Drinking", AnimationId::Knight_Drinking },
		{ "Knight_Death", AnimationId::Knight_Death },
		{ "FinalBoss_Idle", AnimationId::FinalBoss_Idle },
		{ "FinalBoss_Walk", AnimationId::FinalBoss_Walk },
		{ "FinalBoss_Thrust", AnimationId::FinalBoss_Thrust },
		{ "FinalBoss_Slash", AnimationId::FinalBoss_Slash },
		{ "FinalBoss_DashSlash", AnimationId::FinalBoss_DashSlash },
		{ "FinalBoss_JumpSlash", AnimationId::FinalBoss_JumpSlash },
		{ "FinalBoss_MultiSlash", AnimationId::FinalBoss_MultiSlash },
		{ "FinalBoss_Stun", AnimationId::FinalBoss_Stun },
		{ "FinalBoss_Hit", AnimationId::FinalBoss_Hit },
		{ "FinalBoss_Death", AnimationId::FinalBoss_Death },
		{ "Imp_Idle_1", AnimationId::Imp_Idle_1 },
		{ "Imp_Melee_1", AnimationId::Imp_Melee_1 },
		{ "Imp_Melee_2", AnimationId::Imp_Melee_2 },
		{ "Imp_Melee_3", AnimationId::Imp_Melee_3 },
		{ "Imp_Melee_4", AnimationId::Imp_Melee_4 },
		{ "Imp_Melee_5", AnimationId::Imp_Melee_5 },
		{ "Imp_Walk_Forward", AnimationId::Imp_Walk_Forward },
		{ "Imp_Jump_1", AnimationId::Imp_Jump_1 },
		{ "Imp_Stun", AnimationId::Imp_Stun },
		{ "Imp_React_Front", AnimationId::Imp_React_Front },
		{ "Imp_Death_1", AnimationId::Imp_Death_1 },
		{ "DemonStriker_Idle_1", AnimationId::DemonStriker_Idle_1 },
		{ "DemonStriker_Melee_1", AnimationId::DemonStriker_Melee_1 },
		{ "DemonStriker_Melee_2", AnimationId::DemonStriker_Melee_2 },
		{ "DemonStriker_Melee_3", AnimationId::DemonStriker_Melee_3 },
		{ "DemonStriker_Melee_4", AnimationId::DemonStriker_Melee_4 },
		{ "DemonStriker_Gun_Shoot_1", AnimationId::DemonStriker_Gun_Shoot_1 },
		{ "DemonStriker_Gun_Shoot_2", AnimationId::DemonStriker_Gun_Shoot_2 },
		{ "DemonStriker_Gun_Shoot_3", AnimationId::DemonStriker_Gun_Shoot_3 },
		{ "DemonStriker_Gun_Shoot_4", AnimationId::DemonStriker_Gun_Shoot_4 },
		{ "DemonStriker_Walking_1", AnimationId::DemonStriker_Walking_1 },
		{ "DemonStriker_Running_1", AnimationId::DemonStriker_Running_1 },
		{ "DemonStriker_Jump_1", AnimationId::DemonStriker_Jump_1 },
		{ "DemonStriker_Jump_2", AnimationId::DemonStriker_Jump_2 },
		{ "DemonStriker_Stun", AnimationId::DemonStriker_Stun },
		{ "DemonStriker_React_1_Block", AnimationId::DemonStriker_React_1_Block },
		{ "DemonStriker_Death_1", AnimationId::DemonStriker_Death_1 },
		{ "DemonExecutioner_Idle_2", AnimationId::DemonExecutioner_Idle_2 },
		{ "DemonExecutioner_Melee_1", AnimationId::DemonExecutioner_Melee_1 },
		{ "DemonExecutioner_Melee_2", AnimationId::DemonExecutioner_Melee_2 },
		{ "DemonExecutioner_Melee_3", AnimationId::DemonExecutioner_Melee_3 },
		{ "DemonExecutioner_Melee_4", AnimationId::DemonExecutioner_Melee_4 },
		{ "DemonExecutioner_Melee_5", AnimationId::DemonExecutioner_Melee_5 },
		{ "DemonExecutioner_Melee_6", AnimationId::DemonExecutioner_Melee_6 },
		{ "DemonExecutioner_Walk_Forward", AnimationId::DemonExecutioner_Walk_Forward },
		{ "DemonExecutioner_Run", AnimationId::DemonExecutioner_Run },
		{ "DemonExecutioner_Jump_1", AnimationId::DemonExecutioner_Jump_1 },
		{ "DemonExecutioner_Jump_2", AnimationId::DemonExecutioner_Jump_2 },
		{ "DemonExecutioner_Stun", AnimationId::DemonExecutioner_Stun },
		{ "DemonExecutioner_GetHit_1", AnimationId::DemonExecutioner_GetHit_1 },
		{ "DemonExecutioner_Death", AnimationId::DemonExecutioner_Death },

		{ "BigDemonWarrior_Idle_1", AnimationId::BigDemonWarrior_Idle_1, },
		{ "BigDemonWarrior_Idle_2", AnimationId::BigDemonWarrior_Idle_2, },
		{ "BigDemonWarrior_Idle_3", AnimationId::BigDemonWarrior_Idle_3, },
		{ "BigDemonWarrior_Idle_4", AnimationId::BigDemonWarrior_Idle_4, },
		{ "BigDemonWarrior_BattleCry", AnimationId::BigDemonWarrior_BattleCry, },
		{ "BigDemonWarrior_Roaring", AnimationId::BigDemonWarrior_Roaring, },
		{ "BigDemonWarrior_Melee_1", AnimationId::BigDemonWarrior_Melee_1, },
		{ "BigDemonWarrior_Melee_2", AnimationId::BigDemonWarrior_Melee_2, },
		{ "BigDemonWarrior_Melee_3", AnimationId::BigDemonWarrior_Melee_3, },
		{ "BigDemonWarrior_Melee_4", AnimationId::BigDemonWarrior_Melee_4, },
		{ "BigDemonWarrior_Melee_5", AnimationId::BigDemonWarrior_Melee_5, },
		{ "BigDemonWarrior_Melee_6", AnimationId::BigDemonWarrior_Melee_6, },
		{ "BigDemonWarrior_Melee_7", AnimationId::BigDemonWarrior_Melee_7, },
		{ "BigDemonWarrior_Melee_8", AnimationId::BigDemonWarrior_Melee_8, },
		{ "BigDemonWarrior_Walk_Back", AnimationId::BigDemonWarrior_WalkBack, },
		{ "BigDemonWarrior_Walk_Forward", AnimationId::BigDemonWarrior_WalkForward, },
		{ "BigDemonWarrior_Walk_Left", AnimationId::BigDemonWarrior_WalkLeft, },
		{ "BigDemonWarrior_Walk_Right", AnimationId::BigDemonWarrior_WalkRight, },
		{ "BigDemonWarrior_Run", AnimationId::BigDemonWarrior_RunForward, },
		{ "BigDemonWarrior_Turn_Left", AnimationId::BigDemonWarrior_TurnLeft, },
		{ "BigDemonWarrior_Turn_Right", AnimationId::BigDemonWarrior_TurnRight, },
		{ "BigDemonWarrior_Jump", AnimationId::BigDemonWarrior_Jump, },
		{ "BigDemonWarrior_Stun", AnimationId::BigDemonWarrior_Stun, },
		{ "BigDemonWarrior_React_Left", AnimationId::BigDemonWarrior_ReactFromLeft, },
		{ "BigDemonWarrior_React_Right", AnimationId::BigDemonWarrior_ReactFromRight, },
		{ "BigDemonWarrior_React_Gut", AnimationId::BigDemonWarrior_ReactGut, },
		{ "BigDemonWarrior_Death", AnimationId::BigDemonWarrior_Death, },
	};

	const auto it = kValues.find(text);
	if (it == kValues.end())
		return false;

	outValue = it->second;
	return true;
}

bool ParseDefString(std::string_view text, CharacterFeatureFlags& outValue) noexcept
{
	if (text == "None") { outValue = CharacterFeatureFlags::None; return true; }
	if (text == "Replicated") { outValue = CharacterFeatureFlags::Replicated; return true; }
	if (text == "Combatant") { outValue = CharacterFeatureFlags::Combatant; return true; }
	if (text == "Playable") { outValue = CharacterFeatureFlags::Playable; return true; }
	if (text == "AIControlled") { outValue = CharacterFeatureFlags::AIControlled; return true; }
	if (text == "PortalAware") { outValue = CharacterFeatureFlags::PortalAware; return true; }
	if (text == "BuffUser") { outValue = CharacterFeatureFlags::BuffUser; return true; }
	if (text == "BossPhase") { outValue = CharacterFeatureFlags::BossPhase; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	BuffApplyRequirementType& outValue) noexcept
{
	if (text == "None") { outValue = BuffApplyRequirementType::None; return true; }
	if (text == "AlreadyApplied") { outValue = BuffApplyRequirementType::AlreadyApplied; return true; }
	if (text == "NotAlreadyApplied") { outValue = BuffApplyRequirementType::NotAlreadyApplied; return true; }
	if (text == "HasStateFlag") { outValue = BuffApplyRequirementType::HasStateFlag; return true; }
	if (text == "MissingStateFlag") { outValue = BuffApplyRequirementType::MissingStateFlag; return true; }
	if (text == "HpRatioAbove") { outValue = BuffApplyRequirementType::HpRatioAbove; return true; }
	if (text == "HpRatioBelow") { outValue = BuffApplyRequirementType::HpRatioBelow; return true; }
	if (text == "TargetFactionIs") { outValue = BuffApplyRequirementType::TargetFactionIs; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffCounterEventType& outValue) noexcept
{
	if (text == "None") { outValue = BuffCounterEventType::None; return true; }
	if (text == "OwnerDeath") { outValue = BuffCounterEventType::OwnerDeath; return true; }
	if (text == "OwnerKill") { outValue = BuffCounterEventType::OwnerKill; return true; }
	if (text == "OwnerDamaged") { outValue = BuffCounterEventType::OwnerDamaged; return true; }
	if (text == "ActionCommitted") { outValue = BuffCounterEventType::ActionCommitted; return true; }
	if (text == "ParrySuccess") { outValue = BuffCounterEventType::ParrySuccess; return true; }
	if (text == "GuardSuccess") { outValue = BuffCounterEventType::GuardSuccess; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffEffectType& outValue) noexcept
{
	if (text == "None") { outValue = BuffEffectType::None; return true; }
	if (text == "StatAdd") { outValue = BuffEffectType::StatAdd; return true; }
	if (text == "StatMul") { outValue = BuffEffectType::StatMul; return true; }
	if (text == "StateFlag") { outValue = BuffEffectType::StateFlag; return true; }
	if (text == "PeriodicEffect") { outValue = BuffEffectType::PeriodicEffect; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffFamilyId& outValue) noexcept
{
	if (text == "None") { outValue = BuffFamilyId::None; return true; }
	if (text == "HpBoost") { outValue = BuffFamilyId::HpBoost; return true; }
	if (text == "StaminaBoost") { outValue = BuffFamilyId::StaminaBoost; return true; }
	if (text == "AttackBoost") { outValue = BuffFamilyId::AttackBoost; return true; }
	if (text == "AttackSpeedBoost") { outValue = BuffFamilyId::AttackSpeedBoost; return true; }
	if (text == "DefenseBoost") { outValue = BuffFamilyId::DefenseBoost; return true; }
	if (text == "MoveSpeedBoost") { outValue = BuffFamilyId::MoveSpeedBoost; return true; }
	if (text == "ParrySuccess") { outValue = BuffFamilyId::ParrySuccess; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	BuffFamilyStackPolicy& outValue) noexcept
{
	if (text == "Independent") { outValue = BuffFamilyStackPolicy::Independent; return true; }
	if (text == "ReplaceWithHigherTier") { outValue = BuffFamilyStackPolicy::ReplaceWithHigherTier; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffId& outValue) noexcept
{
	if (text == "None") { outValue = BuffId::None; return true; }
	if (text == "HpBoost_Low") { outValue = BuffId::HpBoost_Low; return true; }
	if (text == "HpBoost_Mid") { outValue = BuffId::HpBoost_Mid; return true; }
	if (text == "HpBoost_High") { outValue = BuffId::HpBoost_High; return true; }
	if (text == "StaminaBoost") { outValue = BuffId::StaminaBoost; return true; }
	if (text == "AttackBoost") { outValue = BuffId::AttackBoost; return true; }
	if (text == "AttackSpeedBoost") { outValue = BuffId::AttackSpeedBoost; return true; }
	if (text == "DefenceBoost") { outValue = BuffId::DefenceBoost; return true; }
	if (text == "MoveSpeedBoost") { outValue = BuffId::MoveSpeedBoost; return true; }
	if (text == "ParrySuccess") { outValue = BuffId::ParrySuccess; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffKind& outValue) noexcept
{
	if (text == "None") { outValue = BuffKind::None; return true; }
	if (text == "Positive") { outValue = BuffKind::Positive; return true; }
	if (text == "Negative") { outValue = BuffKind::Negative; return true; }
	if (text == "Control") { outValue = BuffKind::Control; return true; }
	return false;
}

bool ParseDefString(
	std::string_view text,
	BuffModifierConditionType& outValue) noexcept
{
	if (text == "None") { outValue = BuffModifierConditionType::None; return true; }
	if (text == "HasStateFlag") { outValue = BuffModifierConditionType::HasStateFlag; return true; }
	if (text == "MissingStateFlag") { outValue = BuffModifierConditionType::MissingStateFlag; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffModifierTiming& outValue) noexcept
{
	if (text == "Always") { outValue = BuffModifierTiming::Always; return true; }
	if (text == "OnTick") { outValue = BuffModifierTiming::OnTick; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffRemoveRuleType& outValue) noexcept
{
	if (text == "None") { outValue = BuffRemoveRuleType::None; return true; }
	if (text == "OnDurationExpired") { outValue = BuffRemoveRuleType::OnDurationExpired; return true; }
	if (text == "OnOwnerDamaged") { outValue = BuffRemoveRuleType::OnOwnerDamaged; return true; }
	if (text == "OnOwnerActionCommitted") { outValue = BuffRemoveRuleType::OnOwnerActionCommitted; return true; }
	if (text == "OnSpecificActionCommitted") { outValue = BuffRemoveRuleType::OnSpecificActionCommitted; return true; }
	if (text == "OnStackDepleted") { outValue = BuffRemoveRuleType::OnStackDepleted; return true; }
	if (text == "OnStateFlagMissing") { outValue = BuffRemoveRuleType::OnStateFlagMissing; return true; }
	if (text == "OnBuffFamilyApplied") { outValue = BuffRemoveRuleType::OnBuffFamilyApplied; return true; }
	if (text == "OnCounterReached") { outValue = BuffRemoveRuleType::OnCounterReached; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BuffTier& outValue) noexcept
{
	if (text == "None") { outValue = BuffTier::None; return true; }
	if (text == "Low") { outValue = BuffTier::Low; return true; }
	if (text == "Mid") { outValue = BuffTier::Mid; return true; }
	if (text == "High") { outValue = BuffTier::High; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CharacterId& outValue) noexcept
{
	if (text == "None") { outValue = CharacterId::None; return true; }
	if (text == "Knight") { outValue = CharacterId::Knight; return true; }
	if (text == "Lancer") { outValue = CharacterId::Lancer; return true; }
	if (text == "Paladin") { outValue = CharacterId::Paladin; return true; }
	if (text == "Imp") { outValue = CharacterId::Imp; return true; }
	if (text == "DemonStriker") { outValue = CharacterId::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = CharacterId::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = CharacterId::BigDemonWarrior; return true; }
	if (text == "Tank") { outValue = CharacterId::Tank; return true; }
	if (text == "FinalBoss") { outValue = CharacterId::FinalBoss; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CharacterRole& outValue) noexcept
{
	if (text == "Player") { outValue = CharacterRole::Player; return true; }
	if (text == "Monster") { outValue = CharacterRole::Monster; return true; }
	if (text == "Boss") { outValue = CharacterRole::Boss; return true; }
	if (text == "NPC") { outValue = CharacterRole::NPC; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CombatEffectType& outValue) noexcept
{
	if (text == "None") { outValue = CombatEffectType::None; return true; }
	if (text == "AttackHit") { outValue = CombatEffectType::AttackHit; return true; }
	if (text == "ParryResponse") { outValue = CombatEffectType::ParryResponse; return true; }
	if (text == "GuardResponse") { outValue = CombatEffectType::GuardResponse; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CombatReferenceFrame& outValue) noexcept
{
	if (text == "OwnerFacing") { outValue = CombatReferenceFrame::OwnerFacing; return true; }
	if (text == "MoveDirection") { outValue = CombatReferenceFrame::MoveDirection; return true; }
	if (text == "LockedActionDirection") { outValue = CombatReferenceFrame::LockedActionDirection; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CombatWindowType& outValue) noexcept
{
	if (text == "Attack") { outValue = CombatWindowType::Attack; return true; }
	if (text == "Parry") { outValue = CombatWindowType::Parry; return true; }
	if (text == "Guard") { outValue = CombatWindowType::Guard; return true; }
	if (text == "Armor") { outValue = CombatWindowType::Armor; return true; }
	if (text == "Invulnerability") { outValue = CombatWindowType::Invulnerability; return true; }
	return false;
}

bool ParseDefString(std::string_view text, DirectionPolicy& outValue) noexcept
{
	if (text == "ActionStartInput") { outValue = DirectionPolicy::ActionStartInput; return true; }
	if (text == "CurrentInput") { outValue = DirectionPolicy::CurrentInput; return true; }
	if (text == "FacingDirection") { outValue = DirectionPolicy::FacingDirection; return true; }
	if (text == "TargetDirection") { outValue = DirectionPolicy::TargetDirection; return true; }
	if (text == "LockedDirection") { outValue = DirectionPolicy::LockedDirection; return true; }
	return false;
}

bool ParseDefString(std::string_view text, DirectionSampleTiming& outValue) noexcept
{
	if (text == "OnSegmentStart") { outValue = DirectionSampleTiming::OnSegmentStart; return true; }
	if (text == "Continuous") { outValue = DirectionSampleTiming::Continuous; return true; }
	return false;
}

bool ParseDefString(std::string_view text, DurationPolicy& outValue) noexcept
{
	if (text == "Instant") { outValue = DurationPolicy::Instant; return true; }
	if (text == "Timed") { outValue = DurationPolicy::Timed; return true; }
	if (text == "Infinite") { outValue = DurationPolicy::Infinite; return true; }
	return false;
}

bool ParseDefString(std::string_view text, EventType& outValue) noexcept
{
	if (text == "ConsumeItem") { outValue = EventType::ConsumeItem; return true; }
	if (text == "ApplyGameplayEffect") { outValue = EventType::ApplyGameplayEffect; return true; }
	if (text == "SpawnProjectile") { outValue = EventType::SpawnProjectile; return true; }
	if (text == "PlayEffect") { outValue = EventType::PlayEffect; return true; }
	return false;
}

bool ParseDefString(std::string_view text, Faction& outValue) noexcept
{
	if (text == "None") { outValue = Faction::None; return true; }
	if (text == "Neutral") { outValue = Faction::Neutral; return true; }
	if (text == "Player") { outValue = Faction::Player; return true; }
	if (text == "Enemy") { outValue = Faction::Enemy; return true; }
	return false;
}

bool ParseDefString(std::string_view text, GameplayStateFlag& outValue) noexcept
{
	if (text == "None") { outValue = GameplayStateFlag::None; return true; }
	if (text == "SuperArmor") { outValue = GameplayStateFlag::SuperArmor; return true; }
	if (text == "Invulnerable") { outValue = GameplayStateFlag::Invulnerable; return true; }
	if (text == "CannotMove") { outValue = GameplayStateFlag::CannotMove; return true; }
	if (text == "CannotAct") { outValue = GameplayStateFlag::CannotAct; return true; }
	if (text == "ParrySuccessReady") { outValue = GameplayStateFlag::ParrySuccessReady; return true; }
	return false;
}

bool ParseDefString(std::string_view text, HorizontalMovementMode& outValue) noexcept
{
	if (text == "None") { outValue = HorizontalMovementMode::None; return true; }
	if (text == "ForwardFixedDistance") { outValue = HorizontalMovementMode::ForwardFixedDistance; return true; }
	if (text == "InputDirectionDistance") { outValue = HorizontalMovementMode::InputDirectionDistance; return true; }
	return false;
}

bool ParseDefString(std::string_view text, LocomotionMode& outValue) noexcept
{
	if (text == "Idle") { outValue = LocomotionMode::Idle; return true; }
	if (text == "Walk") { outValue = LocomotionMode::Walk; return true; }
	if (text == "Run") { outValue = LocomotionMode::Run; return true; }
	if (text == "Turn") { outValue = LocomotionMode::Turn; return true; }
	if (text == "WalkBack") { outValue = LocomotionMode::WalkBack; return true; }
	if (text == "WalkLeft") { outValue = LocomotionMode::WalkLeft; return true; }
	if (text == "WalkRight") { outValue = LocomotionMode::WalkRight; return true; }
	if (text == "TurnLeft") { outValue = LocomotionMode::TurnLeft; return true; }
	if (text == "TurnRight") { outValue = LocomotionMode::TurnRight; return true; }
	return false;
}

bool ParseDefString(std::string_view text, OverlappingPolicy& outValue) noexcept
{
	if (text == "Reject") { outValue = OverlappingPolicy::Reject; return true; }
	if (text == "Refresh") { outValue = OverlappingPolicy::Refresh; return true; }
	if (text == "Stack") { outValue = OverlappingPolicy::Stack; return true; }
	return false;
}

bool ParseDefString(std::string_view text, ReapplyPolicy& outValue) noexcept
{
	if (text == "None") { outValue = ReapplyPolicy::None; return true; }
	if (text == "RefreshDuration") { outValue = ReapplyPolicy::RefreshDuration; return true; }
	if (text == "RefreshStacks") { outValue = ReapplyPolicy::RefreshStacks; return true; }
	if (text == "RefreshDurationAndStacks") { outValue = ReapplyPolicy::RefreshDurationAndStacks; return true; }
	return false;
}

bool ParseDefString(std::string_view text, RespawnPolicy& outValue) noexcept
{
	if (text == "None") { outValue = RespawnPolicy::None; return true; }
	if (text == "Delay") { outValue = RespawnPolicy::Delay; return true; }
	return false;
}

bool ParseDefString(std::string_view text, RotationMode& outValue) noexcept
{
	if (text == "None") { outValue = RotationMode::None; return true; }
	if (text == "FaceMoveDirection") { outValue = RotationMode::FaceMoveDirection; return true; }
	if (text == "FaceTarget") { outValue = RotationMode::FaceTarget; return true; }
	return false;
}

bool ParseDefString(std::string_view text, SpawnConditionType& outValue) noexcept
{
	if (text == "Always") { outValue = SpawnConditionType::Always; return true; }
	if (text == "OnWorldStart") { outValue = SpawnConditionType::OnWorldStart; return true; }
	if (text == "OnRespawn") { outValue = SpawnConditionType::OnRespawn; return true; }
	return false;
}

bool ParseDefString(std::string_view text, SpawnSetId& outValue) noexcept
{
	if (text == "None") { outValue = SpawnSetId::None; return true; }
	if (text == "PlazaDefault") { outValue = SpawnSetId::PlazaDefault; return true; }
	if (text == "VillageDefault") { outValue = SpawnSetId::VillageDefault; return true; }
	if (text == "CastleDefault") { outValue = SpawnSetId::CastleDefault; return true; }
	if (text == "FinalDefault") { outValue = SpawnSetId::FinalDefault; return true; }
	if (text == "PvpDefault") { outValue = SpawnSetId::PvpDefault; return true; }
	return false;
}

bool ParseDefString(std::string_view text, StatType& outValue) noexcept
{
	if (text == "MaxHp") { outValue = StatType::MaxHp; return true; }
	if (text == "MaxStamina") { outValue = StatType::MaxStamina; return true; }
	if (text == "AttackPower") { outValue = StatType::AttackPower; return true; }
	if (text == "Defense") { outValue = StatType::Defense; return true; }
	if (text == "MoveSpeed") { outValue = StatType::MoveSpeed; return true; }
	if (text == "AttackSpeed") { outValue = StatType::AttackSpeed; return true; }
	return false;
}

bool ParseDefString(std::string_view text, TriggerConditionType& outValue) noexcept
{
	if (text == "Always") { outValue = TriggerConditionType::Always; return true; }
	if (text == "OnParrySuccess") { outValue = TriggerConditionType::OnParrySuccess; return true; }
	return false;
}

bool ParseDefString(std::string_view text, VerticalMovementMode& outValue) noexcept
{
	if (text == "None") { outValue = VerticalMovementMode::None; return true; }
	if (text == "FixedOffset") { outValue = VerticalMovementMode::FixedOffset; return true; }
	return false;
}

bool ParseSpawnPointIdString(std::string_view text, SpawnPointId& outValue) noexcept
{
	if (text == "None") { outValue = SpawnPointIds::None; return true; }
	if (text == "PlazaPlayerStart") { outValue = SpawnPointIds::PlazaPlayerStart; return true; }
	if (text == "PlazaMonster01") { outValue = SpawnPointIds::PlazaMonster01; return true; }
	if (text == "VillagePlayerStart") { outValue = SpawnPointIds::VillagePlayerStart; return true; }
	if (text == "VillageMonster01") { outValue = SpawnPointIds::VillageMonster01; return true; }
	if (text == "VillageMonster02") { outValue = SpawnPointIds::VillageMonster02; return true; }
	if (text == "VillageMonster03") { outValue = SpawnPointIds::VillageMonster03; return true; }
	if (text == "VillageMonster04") { outValue = SpawnPointIds::VillageMonster04; return true; }
	if (text == "VillageMonster05") { outValue = SpawnPointIds::VillageMonster05; return true; }
	if (text == "VillageBossMonster01") { outValue = SpawnPointIds::VillageBossMonster01; return true; }
	if (text == "CastlePlayerStart") { outValue = SpawnPointIds::CastlePlayerStart; return true; }
	if (text == "CastleMonster01") { outValue = SpawnPointIds::CastleMonster01; return true; }
	if (text == "CastleMonster02") { outValue = SpawnPointIds::CastleMonster02; return true; }
	if (text == "CastleMonster03") { outValue = SpawnPointIds::CastleMonster03; return true; }
	if (text == "CastleMonster04") { outValue = SpawnPointIds::CastleMonster04; return true; }
	if (text == "CastleMonster05") { outValue = SpawnPointIds::CastleMonster05; return true; }
	if (text == "CastleMonster06") { outValue = SpawnPointIds::CastleMonster06; return true; }
	if (text == "FinalPlayerStart") { outValue = SpawnPointIds::FinalPlayerStart; return true; }
	if (text == "FinalMonster01") { outValue = SpawnPointIds::FinalMonster01; return true; }
	if (text == "PvpPlayerStartA") { outValue = SpawnPointIds::PvpPlayerStartA; return true; }
	if (text == "PvpPlayerStartB") { outValue = SpawnPointIds::PvpPlayerStartB; return true; }
	return false;
}
