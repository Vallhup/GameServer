#pragma once

#include "World.h"

class IWorldFactory {
public:
	virtual ~IWorldFactory() = default;
	virtual std::unique_ptr<IWorldImpl> CreateImpl(const WorldDesc& desc) = 0;
};