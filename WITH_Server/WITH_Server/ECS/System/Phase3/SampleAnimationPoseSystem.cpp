#include "pch.h"
#include "SampleAnimationPoseSystem.h"

#include <algorithm>
#include <cmath>

#include "../../../AnimationRegistry.h"
#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 3> kSampleAnimationPoseAccesses{
		ReadSnapshot(ComponentRes<AnimationPlaybackStateComp>()),
		WriteImmediate(ComponentRes<SampledAnimationPoseComp>()),
		ReadSnapshot(ExternalRes<AnimationRegistry>()),
	};
}

const SystemMeta SampleAnimationPoseSystem::kMeta =
	SystemMeta{
		SysTag<SampleAnimationPoseSystem>(),
		"SampleAnimationPoseSystem",
		kSampleAnimationPoseAccesses,
		kNoDeps,
		kNoDeps
	};

SampleAnimationPoseSystem::SampleAnimationPoseSystem(
	const AnimationRegistry* animationRegistry)
	: _animationRegistry(animationRegistry)
{
}

void SampleAnimationPoseSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, playbackState, pose] :
		ctx.ecs.View<AnimationPlaybackStateComp, SampledAnimationPoseComp>())
	{
		(void)entity;
		pose.animationId = playbackState.animationId;
		pose.sampleFrameIndex = 0;
		pose.localCapsules.clear();

		if (playbackState.animationId == AnimationId::None ||
			_animationRegistry == nullptr)
		{
			continue;
		}

		const AnimationClipDef* clip =
			_animationRegistry->Find(playbackState.animationId);
		if (clip == nullptr || clip->frames.empty())
		{
			continue;
		}

		const size_t frameCount = clip->frames.size();
		const float normalized = playbackState.loop
			? std::fmod(
				std::max(0.0f, playbackState.normalizedTime),
				1.0f)
			: ClampFloat(playbackState.normalizedTime, 0.0f, 1.0f);
		const float sampleFrame = normalized * static_cast<float>(frameCount);
		const size_t frameIndex = std::min(
			frameCount - 1,
			static_cast<size_t>(sampleFrame));

		pose.sampleFrameIndex = static_cast<uint16_t>(frameIndex);
		pose.localCapsules = clip->frames[frameIndex].capsules;
	}
}

const SystemMeta& SampleAnimationPoseSystem::Meta() const
{
	return kMeta;
}
