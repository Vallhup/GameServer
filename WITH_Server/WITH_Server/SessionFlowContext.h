#pragma once

#include "SessionFlowTypes.h"

class FrameworkRuntime;
class IWorldTransitionRequestSink;
class NetworkRuntime;
class CharacterDataService;
class CharacterSpawnService;
class SessionBindingRegistry;

struct SessionFlowDependencies
{
	NetworkRuntime* network{ nullptr };
	FrameworkRuntime* framework{ nullptr };
	SessionBindingRegistry* sessionBindings{ nullptr };
	CharacterDataService* characterData{ nullptr };
	CharacterSpawnService* characterSpawn{ nullptr };
	IWorldTransitionRequestSink* worldTransitionSink{ nullptr };
};

class SessionFlowContext final {
public:
	SessionFlowContext(SessionFlow& flow, SessionFlowDependencies& dependencies) noexcept
		: _flow(flow), _dependencies(dependencies) {}

	SessionFlow& Flow() noexcept { return _flow; }
	const SessionFlow& Flow() const noexcept { return _flow; }

	SessionFlowDependencies& Dependencies() noexcept { return _dependencies; }
	const SessionFlowDependencies& Dependencies() const noexcept { return _dependencies; }

private:
	SessionFlow& _flow;
	SessionFlowDependencies& _dependencies;
};
