#pragma once

#include <span>
#include <vector>

#include "ITransferContext.h"
#include "WorldTransferTypes.h"

class WorldTransferContext final : public ITransferContext {
public:
	WorldTransferContext() = default;
	~WorldTransferContext() override = default;

	uint32_t PlayerCount() const override
	{
		return static_cast<uint32_t>(_sessionIds.size());
	}

	std::span<const uint32_t> SessionIds() const noexcept override
	{
		return std::span<const uint32_t>(_sessionIds.data(), _sessionIds.size());
	}

	std::span<const TransferEntitySnapshot> Entities() const noexcept
	{
		return std::span<const TransferEntitySnapshot>(_entities.data(), _entities.size());
	}

	std::vector<uint32_t>& MutableSessionIds() noexcept
	{
		return _sessionIds;
	}

	std::vector<TransferEntitySnapshot>& MutableEntities() noexcept
	{
		return _entities;
	}

private:
	std::vector<uint32_t> _sessionIds;
	std::vector<TransferEntitySnapshot> _entities;
};
