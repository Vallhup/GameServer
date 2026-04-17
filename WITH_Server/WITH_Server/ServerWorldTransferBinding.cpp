#include "pch.h"
#include "ServerWorldTransferBinding.h"

#include "FrameworkRuntime.h"
#include "SessionBindingRegistry.h"

ServerWorldTransferBinding::ServerWorldTransferBinding(
	const FrameworkRuntime& framework,
	const SessionBindingRegistry& sessionBindings) noexcept
	: _framework(framework)
	, _sessionBindings(sessionBindings)
{
}

bool ServerWorldTransferBinding::TryResolveRootEntity(
	uint32_t sessionId,
	Entity& outEntity,
	NetId& outNetId) const
{
	outEntity = Entity::Null();
	outNetId = NetId::Invalid();

	const SessionBinding* const binding =
		_sessionBindings.FindBySession(sessionId);
	if (binding == nullptr ||
		!binding->IsValid() ||
		!binding->currentWorldId.IsValid())
	{
		return false;
	}

	const NetBindingLocation location =
		_framework.FindNetBinding(binding->controlledNetId);
	if (!location.IsValid())
	{
		return false;
	}

	if (location.worldId != binding->currentWorldId)
	{
		return false;
	}

	outEntity = location.entity;
	outNetId = binding->controlledNetId;
	return true;
}
