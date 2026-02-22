#pragma once

#include "Component.h"

struct ActionIntent : public Component {
	bool attack{ false };
	bool dodge{ false };
	bool parry{ false };
	bool guard{ false };
};
