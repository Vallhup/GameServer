#include "pch.h"
#include "AnimationSetFactory.h"
#include "AnimationSet.h"

// ---------------------------------------------------------------------
// Code should be sequentially indexed for naming in AnimationSet Class
// ---------------------------------------------------------------------

shared_ptr<AnimationSet> AnimationSetFactory::CreateKnightSet()
{
	auto set = make_shared<AnimationSet>("Knight", 1);

	set->RegisterClip("Idle",	0, AnimCategory::Base);
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

	return set;
}

shared_ptr<AnimationSet> AnimationSetFactory::CreateLancerSet()
{
	auto set = make_shared<AnimationSet>("Lancer", 13);

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
	auto set = make_shared<AnimationSet>("Tanker", 24);

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