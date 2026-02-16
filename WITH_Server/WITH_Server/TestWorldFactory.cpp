#include "pch.h"
#include "TestWorldFactory.h"
#include "TestWorldImpl.h"

std::unique_ptr<IWorldImpl> TestWorldFactory::CreateImpl(const WorldDesc& desc)
{
	return std::make_unique<TestWorldImpl>();
}