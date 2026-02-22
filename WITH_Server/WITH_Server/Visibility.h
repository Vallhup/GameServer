#pragma once

#include <vector>
#include "Component.h"
#include "types.h"

struct ViewList : public Component {
	std::vector<uint32> viewList;
};