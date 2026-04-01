#pragma once

#include <span>

#include "ExecutionCoreTypes.h"

class WorldRuntime;

class WorldManager;
class WorldRegistry;
class WorldTransferService;
class WorldAdmissionService;
class PresenceManager;

class ExecutionOps final {
public:
	ExecutionOps() = default;
	ExecutionOps(
		WorldManager*			worldManager,
		WorldRegistry*			worldRegistry,
		WorldTransferService*	worldTransferService,
		WorldAdmissionService*	worldAdmissionService,
		PresenceManager*		persenceManager
	) noexcept;

	[[nodiscard]]
	WorldRuntime* GetRuntime(
		ExecScopeId scopeId,
		std::span<WorldRuntime*> runtimeByScope
	) noexcept;


	void CommitScope(
		ExecScopeId scopeId,
		std::span<WorldRuntime*> runtimeByScope
	);

	void FlushLifecycle(
		ExecScopeId scopeId,
		std::span<WorldRuntime*> runtimeByScope
	);

	void ReconcileScope(
		ExecScopeId scopeId,
		std::span<WorldRuntime*> runtimeByScope
	);

	[[nodiscard]]
	bool IsValid() const noexcept
	{
		return
			_worldManager			!= nullptr &&
			_worldRegistry			!= nullptr &&
			_worldTransferService	!= nullptr &&
			_worldAdmissionService	!= nullptr &&
			_persenceManager		!= nullptr;
	}

private:
	WorldManager*			_worldManager{ nullptr };
	WorldRegistry*			_worldRegistry{ nullptr };
	WorldTransferService*	_worldTransferService{ nullptr };
	WorldAdmissionService*	_worldAdmissionService{ nullptr };
	PresenceManager*		_persenceManager{ nullptr };
};