#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Animation.h"

class AnimationCommitSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AnimationCommitSystem>(),
		"AnimationCommitSystem",
		std::array{
			ReadImmediate(ComponentRes<AnimationState>()),
			WriteImmediate(ComponentRes<Animator>()),
			WriteImmediate(ComponentRes<CombatCollider>())
		}
	);

public:
	AnimationCommitSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AnimationCommitSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }
	
private:
	void BindColliderToClip(CombatCollider* collider, const PrebakedAnimation& clip);
};
