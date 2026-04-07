#pragma once

#include <array>
#include "SystemMeta.h"
#include "SystemMetaHelper.h"

template<size_t NAccess, size_t NBefore = 0, size_t NAfter = 0>
struct StaticSystemMetaStorage
{
	std::array<AccessSpec, NAccess> accesses;
	std::array<SystemTag, NBefore>	runsBefore;
	std::array<SystemTag, NAfter>	runsAfter;
	SystemMeta meta;

	StaticSystemMetaStorage(
		SystemTag tag,
		std::string_view name,
		std::array<AccessSpec, NAccess> access,
		std::array<SystemTag, NBefore>	before = { },
		std::array<SystemTag, NAfter>	after = { }) 
		: accesses(std::move(access)), 
		runsBefore(std::move(before)), 
		runsAfter(std::move(after)),
		meta{ tag, name, accesses, runsBefore, runsAfter }
	{
	}
};

template<size_t NAccess, size_t NBefore = 0, size_t NAfter = 0>
StaticSystemMetaStorage<NAccess, NBefore, NAfter> MakeMetaStorage(
	SystemTag tag,
	std::string_view name,
	std::array<AccessSpec, NAccess> access,
	std::array<SystemTag, NBefore>	before = { },
	std::array<SystemTag, NAfter>	after = { })
{
	return { tag, name , std::move(access), std::move(before), std::move(after) };
}