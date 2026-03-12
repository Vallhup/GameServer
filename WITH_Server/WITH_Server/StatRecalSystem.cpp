#include "pch.h"
#include "StatRecalSystem.h"
#include "Framework.h"
#include "RepComponent.h""

void StatRecalSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	for (const auto& [entity, baseVital, baseAttr, finalVital, finalAttr, bufComp, vital] :
		ecs.View<BaseVital, BaseAttribute, FinalVital,
		FinalAttribute, BuffsComp, Vital>(
			Exclude<DisconnectedTag>{}))
	{
		const bool needRecalc =
			finalAttr.dirty ||
			finalVital.dirty ||
			bufComp.dirty;

		if (!needRecalc) continue;

		finalAttr.power = baseAttr.power;
		finalAttr.attackSpeed = baseAttr.attackSpeed;
		finalAttr.defense = baseAttr.defense;
		finalAttr.moveSpeed = baseAttr.moveSpeed;

		finalVital.maxHp = baseVital.maxHp;
		finalVital.maxStamina = baseVital.maxStamina;
		finalVital.staminaRecoveryPerSec = baseVital.staminaRecoveryPerSec;

		for (const BuffInstance& inst : bufComp.buffs)
		{
			ApplyBuff(entity, inst, finalVital, finalAttr);
		}
		bufComp.dirty = false;

		vital.curHp = std::clamp(vital.curHp, 0, finalVital.maxHp);
		vital.curStamina = std::clamp(vital.curStamina, 0, finalVital.maxStamina);

		finalAttr.dirty = false;
		finalVital.dirty = false;

		_runtime.MarkDirty(entity, WorldDirtyType::Stat);

#ifdef _DEBUG
		std::printf("MaxHp: %d, MaxStamina: %d",
			finalVital.maxHp, finalVital.maxStamina);
#endif
	}
}

void StatRecalSystem::ApplyBuff(Entity entity, const BuffInstance& inst, FinalVital& fVital, FinalAttribute& fAttr)
{
	switch (inst.type) {
	case BuffType::HpBoost:
	{
		fVital.maxHp += 100;
		break;
	}
	case BuffType::StaminaBoost:
	{
		fVital.maxStamina += 100;
		break;
	}
	}
}
