#pragma once

#include <memory>
#include <unordered_map>

#include "WorldContentIds.h"
#include "WorldTransferProfile.h"

class WorldTransferProfileRegistry final {
public:
	WorldTransferProfileRegistry() = default;
	~WorldTransferProfileRegistry() = default;

	WorldTransferProfileRegistry(const WorldTransferProfileRegistry&) = delete;
	WorldTransferProfileRegistry& operator=(const WorldTransferProfileRegistry&) = delete;
	WorldTransferProfileRegistry(WorldTransferProfileRegistry&&) noexcept = default;
	WorldTransferProfileRegistry& operator=(WorldTransferProfileRegistry&&) noexcept = default;

public:
	bool Register(
		WorldTransferProfileId profileId,
		std::unique_ptr<WorldTransferProfile> profile);

	bool Unregister(WorldTransferProfileId profileId);

	bool Has(WorldTransferProfileId profileId) const noexcept;

	WorldTransferProfile* Find(WorldTransferProfileId profileId) noexcept;
	const WorldTransferProfile* Find(WorldTransferProfileId profileId) const noexcept;

	void Clear() noexcept;

private:
	std::unordered_map<
		WorldTransferProfileId,
		std::unique_ptr<WorldTransferProfile>> _profiles;
};
