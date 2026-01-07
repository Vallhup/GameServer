#include "pch.h"
#include "AnimationManager.h"
#include "json.hpp"

AnimationManager::AnimationManager()
{
	LoadAnimation(AnimationId::Knight_Idle, "../Animation/Knight/knight_animation_idle.json");
	LoadAnimation(AnimationId::Knight_Walk, "../Animation/Knight/knight_animation_walk.json");
	LoadAnimation(AnimationId::Knight_Run, "../Animation/Knight/knight_animation_run.json");
	LoadAnimation(AnimationId::Knight_Attack, "../Animation/Knight/knight_animation_attack.json");
	LoadAnimation(AnimationId::Knight_Death, "../Animation/Knight/knight_animation_death.json");
	LoadAnimation(AnimationId::Knight_Drinking, "../Animation/Knight/knight_animation_drinking.json");
	LoadAnimation(AnimationId::Knight_Guard, "../Animation/Knight/knight_animation_guard.json");
	LoadAnimation(AnimationId::Knight_Hit, "../Animation/Knight/knight_animation_hit.json");
	LoadAnimation(AnimationId::Knight_Parry, "../Animation/Knight/knight_animation_parry.json");
	LoadAnimation(AnimationId::Knight_Roll, "../Animation/Knight/knight_animation_roll.json");
	LoadAnimation(AnimationId::Knight_Stun, "../Animation/Knight/knight_animation_stun.json");
}

void AnimationManager::LoadAnimation(AnimationId id, std::string_view path)
{
	if (_animations.contains(id)) return;

	auto anim = std::make_unique<PrebakedAnimation>(LoadPrebakedAnimation(path));

#ifdef _DEBUG
	if (anim.get() != nullptr)
		std::cout << "Animation Load Success: " << path << std::endl;
#endif

	_animations.try_emplace(id, std::move(anim));
}

const PrebakedAnimation* AnimationManager::GetAnimation(AnimationId id) const
{
	auto it = _animations.find(id);
	if (it != _animations.end()) return it->second.get();
	return nullptr;
}

PrebakedAnimation AnimationManager::LoadPrebakedAnimation(std::string_view path)
{
	PrebakedAnimation anim;

	std::ifstream ifs(path.data());
	if (not ifs.is_open())
		throw std::runtime_error("파일을 열 수 없습니다: " + std::string(path));

	json j;
	ifs >> j;

	anim.fps = j["fps"].get<float>();
	anim.numFrames = j["numFrames"].get<int>();

	const auto& jFrames = j["frames"];
	anim.frames.resize(anim.numFrames);

	for (int frame = 0; frame < anim.numFrames; ++frame)
	{
		const auto& jFrame = jFrames[frame];
		anim.frames[frame].reserve(jFrame.size());

		for (const auto& item : jFrame)
		{
			Capsule capsule;
			capsule.bone = item["bone"].get<int>();

			auto p0 = item["p0"];
			capsule.p0 = XMFLOAT3(p0[0].get<float>(), p0[1].get<float>(), p0[2].get<float>());

			auto p1 = item["p1"];
			capsule.p1 = XMFLOAT3(p1[0].get<float>(), p1[1].get<float>(), p1[2].get<float>());

			capsule.radius = item["radius"].get<float>();

			anim.frames[frame].push_back(capsule);
		}
	}

	return anim;
}
