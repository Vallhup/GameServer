#pragma once

#include <cstdint>
#include <span>

class ITransferContext {
public:
	virtual ~ITransferContext() = default;

	virtual uint32_t PlayerCount() const = 0;
	virtual std::span<const uint32_t> ConnectionIds() const noexcept = 0;
};
