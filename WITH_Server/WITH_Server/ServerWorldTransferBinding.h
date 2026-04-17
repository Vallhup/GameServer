#pragma once

#include "IWorldTransferBinding.h"

class FrameworkRuntime;
class SessionBindingRegistry;

class ServerWorldTransferBinding final : public IWorldTransferBinding {
public:
	ServerWorldTransferBinding(
		const FrameworkRuntime& framework,
		const SessionBindingRegistry& sessionBindings) noexcept;

	bool TryResolveRootEntity(
		uint32_t sessionId,
		Entity& outEntity,
		NetId& outNetId) const override;

private:
	const FrameworkRuntime& _framework;
	const SessionBindingRegistry& _sessionBindings;
};
