#include "pch.h"
#include "WorldTransferProfileRegistry.h"

#include "WorldTransferProfile.h"

bool WorldTransferProfileRegistry::Register(
	WorldTransferProfileId profileId,
	std::unique_ptr<WorldTransferProfile> profile)
{
	if (profileId == InvalidWorldTransferProfileId)
		return false;

	if (!profile)
		return false;

	return _profiles.emplace(profileId, std::move(profile)).second;
}

bool WorldTransferProfileRegistry::Unregister(WorldTransferProfileId profileId)
{
	return _profiles.erase(profileId) > 0;
}

bool WorldTransferProfileRegistry::Has(WorldTransferProfileId profileId) const noexcept
{
	return Find(profileId) != nullptr;
}

WorldTransferProfile* WorldTransferProfileRegistry::Find(
	WorldTransferProfileId profileId) noexcept
{
	auto it = _profiles.find(profileId);
	if (it == _profiles.end())
		return nullptr;

	return it->second.get();
}

const WorldTransferProfile* WorldTransferProfileRegistry::Find(
	WorldTransferProfileId profileId) const noexcept
{
	auto it = _profiles.find(profileId);
	if (it == _profiles.end())
		return nullptr;

	return it->second.get();
}

void WorldTransferProfileRegistry::Clear() noexcept
{
	_profiles.clear();
}
