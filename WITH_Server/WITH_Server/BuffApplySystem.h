#pragma once

#include "System.h"
#include "Event.h"
#include "BuffData.h"
#include "Bufs.h"

class BuffApplySystem : public System {
public:
	BuffApplySystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~BuffApplySystem() = default;

	virtual void Execute(const double dT) override;
	virtual std::vector<std::type_index> ReadResources() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(DeathEvent), typeid(BuffsComp) };
	}

private:
	void FindTargetEntities(const ECS& ecs, std::vector<Entity>& out);
	void GiveBuff(Entity target, BuffType type);
};

