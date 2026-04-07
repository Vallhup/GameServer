#include "pch.h"
#include "AnimationRegistry.h"
#include <cmath>

namespace
{
	constexpr float kDurationToleranceSec = 0.001f;
}

AnimationRegistry::BuildResult AnimationRegistry::Rebuild(std::vector<AnimationClipDef> defs)
{
	BuildResult result{};
	Clear();

	for (const auto& def : defs)
	{
		if (!ValidateDef(def, result.error))
		{
			return result;
		}

		const auto [clipIt, clipInserted] = _clipIdToId.emplace(def.clipId, def.id);
		(void)clipIt;
		if (!clipInserted)
		{
			result.error = "Duplicate animation clipId detected.";
			Clear();
			return result;
		}

		_skeletonToIds[def.skeleton].push_back(def.id);
	}

	std::string registryError;
	if (!_defs.Build(std::move(defs), &registryError))
	{
		result.error = registryError;
		Clear();
		return result;
	}

	result.succeeded = true;
	result.loadedCount = _defs.Size();
	return result;
}

void AnimationRegistry::Clear() noexcept
{
	_defs.Clear();
	_clipIdToId.clear();
	_skeletonToIds.clear();
}

bool AnimationRegistry::Empty() const noexcept
{
	return _defs.Empty();
}

size_t AnimationRegistry::Size() const noexcept
{
	return _defs.Size();
}

const AnimationClipDef* AnimationRegistry::Find(AnimationId id) const noexcept
{
	return _defs.Find(id);
}

const AnimationClipDef& AnimationRegistry::Get(AnimationId id) const
{
	return _defs.Get(id);
}

const AnimationClipDef* AnimationRegistry::FindByClipId(std::string_view clipId) const noexcept
{
	const auto it = _clipIdToId.find(std::string(clipId));
	if (it == _clipIdToId.end())
	{
		return nullptr;
	}

	return Find(it->second);
}

std::span<const AnimationClipDef> AnimationRegistry::GetAll() const noexcept
{
	return _defs.GetAll();
}

std::vector<const AnimationClipDef*> AnimationRegistry::FindBySkeleton(std::string_view skeleton) const
{
	std::vector<const AnimationClipDef*> matches;

	const auto it = _skeletonToIds.find(std::string(skeleton));
	if (it == _skeletonToIds.end())
	{
		return matches;
	}

	matches.reserve(it->second.size());
	for (const AnimationId id : it->second)
	{
		if (const AnimationClipDef* def = Find(id); def != nullptr)
		{
			matches.push_back(def);
		}
	}

	return matches;
}

bool AnimationRegistry::ValidateDef(const AnimationClipDef& def, std::string& outError)
{
	if (def.id == AnimationId::None)
	{
		outError = "AnimationId::None is not allowed.";
		return false;
	}

	if (def.clipId.empty())
	{
		outError = "Animation clipId is required.";
		return false;
	}

	if (def.skeleton.empty())
	{
		outError = "Animation skeleton is required.";
		return false;
	}

	if (def.source.empty())
	{
		outError = "Animation source is required.";
		return false;
	}

	if (def.fps <= 0.0f)
	{
		outError = "Animation fps must be greater than zero.";
		return false;
	}

	if (def.numFrames != def.frames.size())
	{
		outError = "Animation numFrames does not match frame count.";
		return false;
	}

	const float expectedDurationSec = static_cast<float>(def.numFrames) / def.fps;
	if (std::fabs(def.durationSec - expectedDurationSec) > kDurationToleranceSec)
	{
		outError = "Animation durationSec does not match numFrames / fps.";
		return false;
	}

	for (const auto& frame : def.frames)
	{
		if (frame.capsules.size() != def.capsuleDefs.size())
		{
			outError = "Animation frame capsule count does not match capsuleDefs.";
			return false;
		}
	}

	return true;
}
