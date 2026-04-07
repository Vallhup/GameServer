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

	set->RegisterClip("Attack", 3, AnimCategory::Action);
	set->RegisterClip("Dodge",	4, AnimCategory::Action);
	set->RegisterClip("Parry",	5, AnimCategory::Action);
	set->RegisterClip("Stun",	6, AnimCategory::Action);
	set->RegisterClip("Hit",	7, AnimCategory::Action);

	set->RegisterClip("Guard",	8, AnimCategory::Special);
	set->RegisterClip("Drink",	9, AnimCategory::Special);

	set->RegisterClip("Death",	10, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateFinalBossSet()
{
	auto set = make_shared<AnimationSet>("FinalBoss", 12);

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
	auto set = make_shared<AnimationSet>("Imp", 22);

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

shared_ptr<AnimationSet> AnimationSetFactory::CreateLancerSet()
{
	auto set = make_shared<AnimationSet>("Lancer", 46);

	set->RegisterClip("Idle", 0, AnimCategory::Base);
	set->RegisterClip("Walk", 1, AnimCategory::Base);
	set->RegisterClip("Run", 2, AnimCategory::Base);

	set->RegisterClip("Attack", 3, AnimCategory::Action);
	set->RegisterClip("Dodge", 4, AnimCategory::Action);
	set->RegisterClip("Parry", 5, AnimCategory::Action);
	set->RegisterClip("Stun", 6, AnimCategory::Action);
	set->RegisterClip("Hit", 7, AnimCategory::Action);

	set->RegisterClip("Guard", 8, AnimCategory::Special);
	set->RegisterClip("Drink", 9, AnimCategory::Special);

	set->RegisterClip("Death", 10, AnimCategory::Die);

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateTankerSet()
{
	auto set = make_shared<AnimationSet>("Tanker", 57);

	set->RegisterClip("Idle", 0, AnimCategory::Base);
	set->RegisterClip("Walk", 1, AnimCategory::Base);
	set->RegisterClip("Run", 2, AnimCategory::Base);

	set->RegisterClip("Attack", 3, AnimCategory::Action);
	set->RegisterClip("Dodge", 4, AnimCategory::Action);
	set->RegisterClip("Parry", 5, AnimCategory::Action);
	set->RegisterClip("Stun", 6, AnimCategory::Action);
	set->RegisterClip("Hit", 7, AnimCategory::Action);

	set->RegisterClip("Guard", 8, AnimCategory::Special);
	set->RegisterClip("Drink", 9, AnimCategory::Special);

	set->RegisterClip("Death", 10, AnimCategory::Die);

	return set;
}