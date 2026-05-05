#include "pch.h"
#include "PacketHandlerContext.h"

#include <cassert>

static PacketHandlerContext* s_instance{ nullptr };

void PacketHandlerContext::Initialize(PacketHandlerContext& ctx) noexcept
{
	s_instance = &ctx;
}

PacketHandlerContext& PacketHandlerContext::Get() noexcept
{
	assert(s_instance != nullptr);
	return *s_instance;
}
