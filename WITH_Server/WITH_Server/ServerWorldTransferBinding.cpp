#include "pch.h"
#include "ServerWorldTransferBinding.h"

#include "FrameworkRuntime.h"
#include "SessionFlowController.h"

ServerWorldTransferBinding::ServerWorldTransferBinding(
	const FrameworkRuntime& framework,
	const SessionFlowController& sessionFlow) noexcept
	: _framework(framework)
	, _sessionFlow(sessionFlow)
{
}

bool ServerWorldTransferBinding::TryResolveRootEntity(
	uint32_t sessionId,
	Entity& outEntity,
	NetId& outNetId) const
{
	outEntity = Entity::Null();
	outNetId  = NetId::Invalid();

	const NetId  controlledNetId = _sessionFlow.FindControlledNetId(sessionId);
	const WorldId currentWorldId = _sessionFlow.FindCurrentWorldId(sessionId);

	if (!controlledNetId.IsValid() || !currentWorldId.IsValid())
	{
		return false;
	}

	const NetBindingLocation location =
		_framework.FindNetBinding(controlledNetId);
	if (!location.IsValid())
	{
		return false;
	}

	if (location.worldId != currentWorldId)
	{
		return false;
	}

	outEntity = location.entity;
	outNetId  = controlledNetId;
	return true;
}
