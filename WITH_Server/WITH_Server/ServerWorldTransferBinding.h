#pragma once

#include "IWorldTransferBinding.h"

class FrameworkRuntime;
class SessionFlowController;

class ServerWorldTransferBinding final : public IWorldTransferBinding {
public:
	ServerWorldTransferBinding(
		const FrameworkRuntime& framework,
		const SessionFlowController& sessionFlow) noexcept;

	bool TryResolveRootEntity(
		uint32_t sessionId,
		Entity& outEntity,
		NetId& outNetId) const override;

private:
	const FrameworkRuntime&      _framework;
	const SessionFlowController& _sessionFlow;
};
