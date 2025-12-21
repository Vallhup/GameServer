#pragma once

#include <typeindex>
#include <vector>
#include <atomic>

#include "Event.h"

struct ECS;

class System {
public:
	System(ECS& e, int p = 0) : ecs(e), _priority(p) {}
	virtual ~System() = default;

	virtual void Execute(const float dT) = 0;

	virtual std::vector<std::type_index> ReadComponents() const { return {}; }
	virtual std::vector<std::type_index> WriteComponents() const { return {}; }

	int GetPriority() const { return _priority; }

protected:
	ECS& ecs;
	int _priority;
};

//
// System Phase 구조
//
// [Phase 0] Input Phase
// [Phase 1] Action State Phase
// [Phase 2] Action Time Phase
// [Phase 3] Movement Phase
// [Phase 4] Combat Window Phase
// [Phase 5] Collision Phase
// [Phase 6] Combat Logic Phase
// [Phase 7] Output Phase
// 
// Action State가 현재 상태와 시간 축 관리
// Combat, Animation 등은 Action State를 참조받아서 동작
//