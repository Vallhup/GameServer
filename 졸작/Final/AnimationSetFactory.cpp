#include "pch.h"
#include "AnimationSetFactory.h"
#include "AnimationSet.h"

// ---------------------------------------------------------------------
// Code should be sequentially indexed for naming in AnimationSet Class
// ---------------------------------------------------------------------

shared_ptr<AnimationSet> AnimationSetFactory::CreateKnightSet()
{
	auto set = make_shared<AnimationSet>("Knight", 1);

	set->RegisterClip("Idle",	0, AnimCategory::Base, 0.05f);
	set->RegisterClip("Walk",	1, AnimCategory::Base);
	set->RegisterClip("Run",	2, AnimCategory::Base);

	set->RegisterClip("AttackCombo1", 3, AnimCategory::Action);
	set->RegisterClip("AttackCombo2", 4, AnimCategory::Action);
	set->RegisterClip("AttackCombo3", 5, AnimCategory::Action);
	set->RegisterClip("AttackStrong", 6, AnimCategory::Action);
	set->RegisterClip("AttackSpecial", 7, AnimCategory::Action);

	set->RegisterClip("Dodge",	8, AnimCategory::Action);
	set->RegisterClip("Parry",	9, AnimCategory::Action);
	set->RegisterClip("Stun",	10, AnimCategory::Action);
	set->RegisterClip("Hit",	11, AnimCategory::Action);

	set->RegisterClip("Guard",	12, AnimCategory::Special);
	set->RegisterClip("Drink",	13, AnimCategory::Special);

	set->RegisterClip("Death",	14, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateFinalBossSet()
{
	auto set = make_shared<AnimationSet>("FinalBoss", 16);

	set->RegisterClip("Idle", 0, AnimCategory::Base);
	set->RegisterClip("Walk", 1, AnimCategory::Base);

	set->RegisterClip("Thrust", 2, AnimCategory::Action);
	set->RegisterClip("Slash", 3, AnimCategory::Action);
	set->RegisterClip("DashSlash", 4, AnimCategory::Action);
	set->RegisterClip("JumpSlash", 5, AnimCategory::Action);
	set->RegisterClip("MultiSlash", 6, AnimCategory::Action);
	set->RegisterClip("Stun", 7, AnimCategory::Action);
	set->RegisterClip("Hit", 8, AnimCategory::Action);

	set->RegisterClip("Death", 9, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateImpSet()
{
	auto set = make_shared<AnimationSet>("Imp", 26);

	set->RegisterClip("Idle1", 0, AnimCategory::Base);
	set->RegisterClip("Idle2", 1, AnimCategory::Base);
	set->RegisterClip("Idle3", 2, AnimCategory::Base);
	set->RegisterClip("Idle4", 3, AnimCategory::Base);
	set->RegisterClip("Idle5", 4, AnimCategory::Base);
	set->RegisterClip("Idle6", 5, AnimCategory::Base);
	set->RegisterClip("BattleCry", 6, AnimCategory::Base);
	set->RegisterClip("Roaring", 7, AnimCategory::Base);

	set->RegisterClip("Melee1", 8, AnimCategory::Action);
	set->RegisterClip("Melee2", 9, AnimCategory::Action);
	set->RegisterClip("Melee3", 10, AnimCategory::Action);
	set->RegisterClip("Melee4", 11, AnimCategory::Action);
	set->RegisterClip("Melee5", 12, AnimCategory::Action);

	set->RegisterClip("WalkBack", 13, AnimCategory::Action);
	set->RegisterClip("WalkForward", 14, AnimCategory::Action);
	set->RegisterClip("WalkLeft", 15, AnimCategory::Action);
	set->RegisterClip("WalkRight", 16, AnimCategory::Action);

	set->RegisterClip("Jump1", 17, AnimCategory::Action);

	set->RegisterClip("Stun", 18, AnimCategory::Action);

	set->RegisterClip("ReactFront", 19, AnimCategory::Action);
	set->RegisterClip("ReactLeft", 20, AnimCategory::Action);
	set->RegisterClip("ReactRight", 21, AnimCategory::Action);
	
	set->RegisterClip("Death1", 22, AnimCategory::Die);
	set->RegisterClip("Death2", 23, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateDemonStrikerSet()
{
	auto set = make_shared<AnimationSet>("DemonStriker", 50);

	set->RegisterClip("Idle1", 0, AnimCategory::Base);
	set->RegisterClip("Idle2", 1, AnimCategory::Base);
	set->RegisterClip("Idle3", 2, AnimCategory::Base);
	set->RegisterClip("Idle4", 3, AnimCategory::Base);
	set->RegisterClip("Idle5", 4, AnimCategory::Base);
	set->RegisterClip("Idle6_Block", 5, AnimCategory::Base);
	
	set->RegisterClip("Melee1", 6, AnimCategory::Action);
	set->RegisterClip("Melee2", 7, AnimCategory::Action);
	set->RegisterClip("Melee3", 8, AnimCategory::Action);
	set->RegisterClip("Melee4", 9, AnimCategory::Action);

	set->RegisterClip("GunShoot1", 10, AnimCategory::Action);
	set->RegisterClip("GunShoot2", 11, AnimCategory::Action);
	set->RegisterClip("GunShoot3", 12, AnimCategory::Action);
	set->RegisterClip("GunShoot4", 13, AnimCategory::Action);

	set->RegisterClip("Walking1", 14, AnimCategory::Action);
	set->RegisterClip("Walking2", 15, AnimCategory::Action);
	set->RegisterClip("Walking3", 16, AnimCategory::Action);
	set->RegisterClip("Walking4_Gun", 17, AnimCategory::Action);
	set->RegisterClip("Walking5_Gun", 18, AnimCategory::Action);
	set->RegisterClip("Walking6_Block", 19, AnimCategory::Action);
	set->RegisterClip("WalkingBack", 20, AnimCategory::Action);
	set->RegisterClip("WalkingLeft", 21, AnimCategory::Action);
	set->RegisterClip("WalkingRight", 22, AnimCategory::Action);

	set->RegisterClip("TurnLeft", 23, AnimCategory::Action);
	set->RegisterClip("TurnRight", 24, AnimCategory::Action);

	set->RegisterClip("Running1", 25, AnimCategory::Action);
	set->RegisterClip("Running2", 26, AnimCategory::Action);

	set->RegisterClip("Jump1", 27, AnimCategory::Action);
	set->RegisterClip("Jump2", 28, AnimCategory::Action);

	set->RegisterClip("Stun", 29, AnimCategory::Action);

	set->RegisterClip("ReactBlock", 30, AnimCategory::Action);
	set->RegisterClip("ReactHeadShot", 31, AnimCategory::Action);
	set->RegisterClip("ReactLeft", 32, AnimCategory::Action);
	set->RegisterClip("ReactRight", 33, AnimCategory::Action);

	set->RegisterClip("Death1", 34, AnimCategory::Die);
	set->RegisterClip("Death2", 35, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateDemonExecutionerSet()
{
	auto set = make_shared<AnimationSet>("DemonExecutioner", 86);

	set->RegisterClip("Idle1", 0, AnimCategory::Base);
	set->RegisterClip("Idle2", 1, AnimCategory::Base);
	set->RegisterClip("Idle3", 2, AnimCategory::Base);
	set->RegisterClip("Idle4", 3, AnimCategory::Base);
	set->RegisterClip("Roar1", 4, AnimCategory::Base);
	set->RegisterClip("Roar2", 5, AnimCategory::Base);

	set->RegisterClip("Melee1", 6, AnimCategory::Action);
	set->RegisterClip("Melee2", 7, AnimCategory::Action);
	set->RegisterClip("Melee3", 8, AnimCategory::Action);
	set->RegisterClip("Melee4", 9, AnimCategory::Action);
	set->RegisterClip("Melee5", 10, AnimCategory::Action);
	set->RegisterClip("Melee6", 11, AnimCategory::Action);

	set->RegisterClip("WalkForward", 12, AnimCategory::Action);
	set->RegisterClip("WalkForwardSlow", 13, AnimCategory::Action);
	set->RegisterClip("WalkLeft", 14, AnimCategory::Action);
	set->RegisterClip("WalkRight", 15, AnimCategory::Action);
	set->RegisterClip("WalkBack", 16, AnimCategory::Action);

	set->RegisterClip("Run", 17, AnimCategory::Action);

	set->RegisterClip("Jump1", 18, AnimCategory::Action);
	set->RegisterClip("Jump2", 19, AnimCategory::Action);

	set->RegisterClip("Stun", 20, AnimCategory::Action);

	set->RegisterClip("Charge", 21, AnimCategory::Action);

	set->RegisterClip("Block1", 22, AnimCategory::Action);
	set->RegisterClip("Block2", 23, AnimCategory::Action);

	set->RegisterClip("GetHit1", 24, AnimCategory::Action);
	set->RegisterClip("GetHit2", 25, AnimCategory::Action);
	set->RegisterClip("GetHit3", 26, AnimCategory::Action);

	set->RegisterClip("Death", 27, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateBigDemonWarriorSet()
{
	auto set = make_shared<AnimationSet>("BigDemonWarrior", 114);

	set->RegisterClip("Idle1", 0, AnimCategory::Base);
	set->RegisterClip("Idle2", 1, AnimCategory::Base);
	set->RegisterClip("Idle3", 2, AnimCategory::Base);
	set->RegisterClip("Idle4", 3, AnimCategory::Base);
	set->RegisterClip("BattleCry", 4, AnimCategory::Base);
	set->RegisterClip("Roaring", 5, AnimCategory::Base);

	set->RegisterClip("Melee1", 6, AnimCategory::Action);
	set->RegisterClip("Melee2", 7, AnimCategory::Action);
	set->RegisterClip("Melee3", 8, AnimCategory::Action);
	set->RegisterClip("Melee4", 9, AnimCategory::Action);
	set->RegisterClip("Melee5", 10, AnimCategory::Action);
	set->RegisterClip("Melee6", 11, AnimCategory::Action);
	set->RegisterClip("Melee7", 12, AnimCategory::Action);
	set->RegisterClip("Melee8", 13, AnimCategory::Action);

	set->RegisterClip("WalkBack", 14, AnimCategory::Action);
	set->RegisterClip("WalkForward", 15, AnimCategory::Action);
	set->RegisterClip("WalkRightBack", 16, AnimCategory::Action);
	set->RegisterClip("WalkRight", 17, AnimCategory::Action);

	set->RegisterClip("RunForward", 18, AnimCategory::Action);

	set->RegisterClip("TurnLeft", 19, AnimCategory::Action);
	set->RegisterClip("TurnRight", 20, AnimCategory::Action);

	set->RegisterClip("Jump", 21, AnimCategory::Action);

	set->RegisterClip("Stun", 22, AnimCategory::Action);

	set->RegisterClip("ReactFromLeft", 23, AnimCategory::Action);
	set->RegisterClip("ReactFromRight", 24, AnimCategory::Action);
	set->RegisterClip("ReactGut", 25, AnimCategory::Action);

	set->RegisterClip("Death", 26, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateTankSet()
{
	auto set = make_shared<AnimationSet>("Tank", 141);

	set->RegisterClip("Idle1", 0, AnimCategory::Base);
	set->RegisterClip("Idle2", 1, AnimCategory::Base);
	set->RegisterClip("Idle3", 2, AnimCategory::Base);
	set->RegisterClip("Idle4", 3, AnimCategory::Base);
	set->RegisterClip("Idle5", 4, AnimCategory::Base);

	set->RegisterClip("Melee1", 5, AnimCategory::Action);
	set->RegisterClip("Melee2", 6, AnimCategory::Action);
	set->RegisterClip("Melee3", 7, AnimCategory::Action);
	set->RegisterClip("Melee4", 8, AnimCategory::Action);
	set->RegisterClip("Melee5", 9, AnimCategory::Action);
	set->RegisterClip("Melee6", 10, AnimCategory::Action);
	set->RegisterClip("Melee7", 11, AnimCategory::Action);
	set->RegisterClip("Melee8", 12, AnimCategory::Action);

	set->RegisterClip("Walk1", 13, AnimCategory::Action);
	set->RegisterClip("Walk2", 14, AnimCategory::Action);
	set->RegisterClip("WalkBack", 15, AnimCategory::Action);
	set->RegisterClip("WalkLeftBack", 16, AnimCategory::Action);
	set->RegisterClip("WalkLeft", 17, AnimCategory::Action);
	set->RegisterClip("WalkRightBack", 18, AnimCategory::Action);
	set->RegisterClip("WalkRight", 19, AnimCategory::Action);

	set->RegisterClip("TurnLeft", 20, AnimCategory::Action);
	set->RegisterClip("TurnRight", 21, AnimCategory::Action);

	set->RegisterClip("Running1", 22, AnimCategory::Action);
	set->RegisterClip("Running2", 23, AnimCategory::Action);

	set->RegisterClip("Jump1", 24, AnimCategory::Action);
	set->RegisterClip("Jump2", 25, AnimCategory::Action);

	set->RegisterClip("Stun", 26, AnimCategory::Action);

	set->RegisterClip("Death1", 27, AnimCategory::Die);
	set->RegisterClip("Death2", 28, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateLancerSet()
{
	auto set = make_shared<AnimationSet>("Lancer", 170);

	set->RegisterClip("Idle", 0, AnimCategory::Base, 0.05f);
	set->RegisterClip("Walk", 1, AnimCategory::Base);
	set->RegisterClip("Run", 2, AnimCategory::Base);

	set->RegisterClip("AttackCombo1", 3, AnimCategory::Action);
	set->RegisterClip("AttackCombo2", 4, AnimCategory::Action);
	set->RegisterClip("AttackCombo3", 5, AnimCategory::Action);
	set->RegisterClip("AttackStrong", 6, AnimCategory::Action);
	set->RegisterClip("AttackSpecial", 7, AnimCategory::Action);
	set->RegisterClip("Dodge", 8, AnimCategory::Action);
	set->RegisterClip("Parry", 9, AnimCategory::Action);
	set->RegisterClip("StunLeft", 10, AnimCategory::Action);
	set->RegisterClip("StunRight", 11, AnimCategory::Action);
	set->RegisterClip("Hit", 12, AnimCategory::Action);

	set->RegisterClip("Guard", 13, AnimCategory::Special);
	set->RegisterClip("Drink", 14, AnimCategory::Special);

	set->RegisterClip("Death", 15, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreatePaladinSet()
{
	auto set = make_shared<AnimationSet>("Paladin", 186);

	set->RegisterClip("Idle", 0, AnimCategory::Base, 0.05f);
	set->RegisterClip("Walk", 1, AnimCategory::Base);
	set->RegisterClip("Run", 2, AnimCategory::Base);

	set->RegisterClip("AttackCombo1", 3, AnimCategory::Action);
	set->RegisterClip("AttackCombo2", 4, AnimCategory::Action);
	set->RegisterClip("AttackCombo3", 5, AnimCategory::Action);
	set->RegisterClip("AttackStrong", 6, AnimCategory::Action);
	set->RegisterClip("AttackSpecial", 7, AnimCategory::Action);
	set->RegisterClip("Dodge", 8, AnimCategory::Action);
	set->RegisterClip("Parry", 9, AnimCategory::Action);
	set->RegisterClip("Stun", 10, AnimCategory::Action);
	set->RegisterClip("Hit", 11, AnimCategory::Action);

	set->RegisterClip("Guard", 12, AnimCategory::Special);
	set->RegisterClip("Drink", 13, AnimCategory::Special);

	set->RegisterClip("Death", 14, AnimCategory::Die);

	return set;
}