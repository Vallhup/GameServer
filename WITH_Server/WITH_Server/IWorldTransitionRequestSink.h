#pragma once

#include <cstdint>

#include "Session.h"
#include "WorldIds.h"

class IWorldTransitionRequestSink {
public:
	virtual ~IWorldTransitionRequestSink() = default;

	virtual TransferId RequestDemoWorldTransition(
		SessionId sessionId,
		uint32_t requestId) = 0;

	virtual bool MarkClientWorldTransitionReady(
		SessionId sessionId,
		TransferId transferId) = 0;

	virtual void OnSessionDisconnected(SessionId sessionId) noexcept = 0;
};
