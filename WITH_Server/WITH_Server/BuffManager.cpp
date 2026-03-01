#include "pch.h"
#include "BuffManager.h"

void BuffManager::LoadBuffDatas(std::string_view path)
{
	BuffDef hpBoost
	{
		.type = BuffType::HpBoost,
		.policy = BuffPolicy::Stacks,
		.effect = BuffEffect::AddStat
	};
	SetBuff(hpBoost);

	BuffDef staminaBoost
	{
		.type = BuffType::StaminaBoost,
		.policy = BuffPolicy::Stacks,
		.effect = BuffEffect::AddStat
	};
	SetBuff(staminaBoost);

	BuffDef attackBoost
	{
		.type = BuffType::AttackBoost,
		.policy = BuffPolicy::Stacks,
		.effect = BuffEffect::AddStat
	};
	SetBuff(attackBoost);

	BuffDef AttackSpeedBoost
	{
		.type = BuffType::AttackSpeedBoost,
		.policy = BuffPolicy::Stacks,
		.effect = BuffEffect::AddStat
	};
	SetBuff(AttackSpeedBoost);

	BuffDef defenceBoost
	{
		.type = BuffType::DefenceBoost,
		.policy = BuffPolicy::Stacks,
		.effect = BuffEffect::AddStat
	};
	SetBuff(defenceBoost);

	BuffDef moveSpeedBoost
	{
		.type = BuffType::MoveSpeedBoost,
		.policy = BuffPolicy::Stacks,
		.effect = BuffEffect::AddStat
	};
	SetBuff(moveSpeedBoost);
}

const BuffDef& BuffManager::GetBuff(BuffType type) const
{
	const size_t index = static_cast<size_t>(type);
	return _buffTable[index];
}

void BuffManager::SetBuff(const BuffDef& buff)
{
	assert((size_t)buff.type > size_t(BuffType::None) &&
		(size_t)buff.type < size_t(BuffType::Count));

	const size_t index = static_cast<size_t>(buff.type);
	_buffTable[index] = buff;
}
