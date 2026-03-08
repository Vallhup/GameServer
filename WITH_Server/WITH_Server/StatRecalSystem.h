#pragma once

#include "System.h"
#include "Stats.h"
#include "Bufs.h"
#include "Tags.h"

class StatRecalSystem : public System {
public:
	StatRecalSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~StatRecalSystem() = default;

	virtual void Execute(const double dT) override;
	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(BaseVital), typeid(BaseAttribute) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(FinalVital), typeid(FinalAttribute),
		typeid(BuffsComp), typeid(Vital) };
	}

private:
	void ApplyBuff(Entity entity, const BuffInstance& inst, FinalVital& fVital, FinalAttribute& fAttr);
};

