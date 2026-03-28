#pragma once

#include <cstdint>
#include <vector>

class ITransferContext {
public:
	virtual ~ITransferContext() = default;

	virtual uint32_t PlayerCount() const = 0;
	virtual const std::vector<uint32_t>& ConnectionIds() const = 0;
};