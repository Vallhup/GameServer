#pragma once

#include "NetIdRegistry.h"
#include "IConnContext.h"

struct ServerContext
{
	NetIdRegistry netIdRegistry;

	std::unique_ptr<IConnContext> connContext;
	std::unique_ptr<IWorldFactory> worldFactory;
};