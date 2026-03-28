#pragma once

#include <memory>

#include "WorldDef.h"
#include "WorldInstance.h"

class IWorldInstanceFactory {
public:
	virtual ~IWorldInstanceFactory() = default;

	virtual std::unique_ptr<IWorldInstanceImpl> Create(const WorldDef& def) = 0;
};