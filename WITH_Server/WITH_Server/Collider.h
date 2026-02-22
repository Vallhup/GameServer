#pragma once

#include <vector>
#include "Component.h"
#include "AnimationData.h"
#include "types.h"

struct CombatCollider : public Component {
	const std::vector<StaticCapsuleData>* staticDatas{ nullptr };
	std::vector<DynamicCapsuleData> localDatas;
	std::vector<DynamicCapsuleData> worldDatas;

	std::vector<uint8> enabledMasks;
	std::vector<uint32> attackIds;
};