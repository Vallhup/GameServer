#pragma once

#include "Entity.h"
#include "Component.h"

struct AIState : public Component {
	Entity target;
	Entity lastAttacker;
	int patternsOnTarget;
};

struct AIThinkState : public Component {
	double thinkAcc{ 0.0f };
	double thinkInterval{ 5.0f };
};