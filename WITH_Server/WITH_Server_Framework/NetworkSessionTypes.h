#pragma once

#include <cstdint>

using SessionId = uint32_t;

enum class SessionCloseReason : uint8_t
{
	None,
	RemoteClosed,
	LocalRequested,
	ProtocolError,
	Timeout,
	ServerShutdown,
};
