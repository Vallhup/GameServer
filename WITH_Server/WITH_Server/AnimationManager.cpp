#include "pch.h"
#include "AnimationManager.h"
#include "json.hpp"

void AnimationManager::LoadAnimation(AnimationType type, std::string_view path)
{
	if (_animations.contains(type)) return;

	auto anim = std::make_unique<PrebakedAnimation>(LoadPrebakedAnimation(path));

#ifdef _DEBUG
	if (anim.get() != nullptr)
		std::cout << "Animation Load Success: " << path << std::endl;
#endif

	_animations.try_emplace(type, std::move(anim));
}

const PrebakedAnimation* AnimationManager::GetAnimation(AnimationType type) const
{
	auto it = _animations.find(type);
	if (it != _animations.end()) return it->second.get();
	return nullptr;
}

std::pair<AnimationType, bool> AnimationManager::
GetAnimationIdForAction(ActionType action) const
{
	auto it = _actionToAnimationMap.find(action);
	if (it != _actionToAnimationMap.end()) return it->second;
	return { AnimationType::None, false };
}

void AnimationManager::LoadActionAnimationMap()
{
	_actionToAnimationMap[ActionType::None] = { AnimationType::None, false };
	_actionToAnimationMap[ActionType::Attack] = { AnimationType::Knight_Attack, false };
	_actionToAnimationMap[ActionType::Dodge] = { AnimationType::Knight_Dodge, false };
	_actionToAnimationMap[ActionType::Parry] = { AnimationType::Knight_Parry, false };
	_actionToAnimationMap[ActionType::Guard] = { AnimationType::Knight_Guard, true };
	_actionToAnimationMap[ActionType::Stun] = { AnimationType::Knight_Stun, false };
	_actionToAnimationMap[ActionType::Hit] = { AnimationType::Knight_Hit, false };
	_actionToAnimationMap[ActionType::Dead] = { AnimationType::Knight_Dead, false };
}

PrebakedAnimation AnimationManager::LoadPrebakedAnimation(std::string_view path)
{
	auto RolesToMask =
		[](const json& rolesArray) -> uint8_t
		{
			uint8 mask = static_cast<uint8>(HitboxType::None);
			if (!rolesArray.is_array()) return mask;

			for (const auto& role : rolesArray)
			{
				if (!role.is_string()) continue;
				const std::string s = role.get<std::string>();

				if (s == "hurt")		mask |= static_cast<uint8>(HitboxType::Hurt);
				else if (s == "hit")	mask |= static_cast<uint8>(HitboxType::Hit);
			}

			return mask;
		};


	PrebakedAnimation anim;

	std::ifstream ifs(path.data());
	if (not ifs.is_open())
		throw std::runtime_error("파일을 열 수 없습니다: " + std::string(path));

	json j;
	ifs >> j;

	const int version = j.value("version", 1);
	if(version != 2)
		throw std::runtime_error("지원하지 않는 애니메이션 버전: " + std::to_string(version));

	anim.fps = j.at("fps").get<double>();

	const int numFrames = j.at("numFrames").get<int>();
	if(numFrames <= 0 || numFrames > 255)
		throw std::runtime_error("잘못된 프레임 수: " + std::to_string(numFrames));
	anim.numFrames = static_cast<uint8>(numFrames);

	const auto& jCaps = j.at("capsules");
	if(!jCaps.is_array())
		throw std::runtime_error("캡슐 데이터가 배열이 아님");

	anim.staticDatas.resize(jCaps.size());
	for (uint64 i = 0; i < jCaps.size(); ++i)
	{
		const auto& jCap = jCaps[i];

		const int boneIndex = jCap.at("bone").get<int>();
		if(boneIndex < 0 || boneIndex > 255)
			throw std::runtime_error("잘못된 본 인덱스: " + std::to_string(boneIndex));

		StaticCapsuleData sCapData;
		sCapData.bone = static_cast<uint8>(boneIndex);
		sCapData.radius = jCap.at("radius").get<double>() * 0.01f;
		sCapData.typeMask = RolesToMask(jCap.value("roles", json::array()));

		anim.staticDatas[i] = sCapData;
	}

	const auto& jFrames = j.at("frames");
	if(!jFrames.is_array())
		throw std::runtime_error("프레임 데이터가 배열이 아님");

	if(jFrames.size() != anim.numFrames)
		throw std::runtime_error("프레임 수 불일치");

	const uint64 capsuleCount = anim.staticDatas.size();
	anim.dynamicDatas.resize(anim.numFrames);

	for (int frame = 0; frame < anim.numFrames; ++frame)
	{
		const auto& jFrame = jFrames[frame];
		if(!jFrame.is_array())
			throw std::runtime_error("프레임 데이터가 배열이 아님: " + std::to_string(frame));

		if(jFrame.size() != capsuleCount)
			throw std::runtime_error("캡슐 수 불일치: " + std::to_string(frame));

		anim.dynamicDatas[frame].resize(capsuleCount);
		for (uint64 i = 0; i < capsuleCount; ++i)
		{
			const auto& item = jFrame[i];

			DynamicCapsuleData dCapData;
			dCapData.p0 = XMFLOAT3(
				item.at("p0")[0].get<double>() * 0.01f,
				item.at("p0")[1].get<double>() * 0.01f,
				item.at("p0")[2].get<double>() * 0.01f);

			dCapData.p1 = XMFLOAT3(
				item.at("p1")[0].get<double>() * 0.01f,
				item.at("p1")[1].get<double>() * 0.01f,
				item.at("p1")[2].get<double>() * 0.01f);

			anim.dynamicDatas[frame][i] = dCapData;
		}
	}

	return anim;
}
