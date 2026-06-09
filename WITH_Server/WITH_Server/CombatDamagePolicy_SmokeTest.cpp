#include "pch.h"

#include <cassert>
#include <iostream>

#include "ECS/System/Phase8/CombatDamagePolicy.h"

void RunCombatDamagePolicySmokeTests()
{
	AbilityGuardResponseDef guardEffect{};
	guardEffect.damageReductionRatio = 0.85f;
	guardEffect.chipDamageRatio = 0.10f;

	assert(CombatDamagePolicy::ResolveGuardedHpDamage(
		100,
		0,
		&guardEffect) == 15);
	assert(CombatDamagePolicy::ResolveGuardedHpDamage(
		7,
		12,
		&guardEffect) == 1);

	guardEffect.damageReductionRatio = 0.95f;
	assert(CombatDamagePolicy::ResolveGuardedHpDamage(
		100,
		0,
		&guardEffect) == 10);

	assert(CombatDamagePolicy::ResolveGuardedHpDamage(
		100,
		0,
		nullptr) == 100);

	std::cout << "[PASS] CombatDamagePolicy smoke\n";
}
