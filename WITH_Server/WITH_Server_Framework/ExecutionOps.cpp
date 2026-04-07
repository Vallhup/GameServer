#include "pch.h"
#include "ExecutionOps.h"

#include <stdexcept>

#include "WorldRuntime.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldTransferService.h"
#include "WorldAdmissionService.h"
#include "PresenceManager.h"

ExecutionOps::ExecutionOps(
	WorldManager*			worldManager,
	WorldRegistry*			worldRegistry, 
	WorldTransferService*	worldTransferService, 
	WorldAdmissionService*	worldAdmissionService, 
	PresenceManager*		persenceManager) noexcept
	: _worldManager(worldManager)
	, _worldRegistry(worldRegistry)
	, _worldTransferService(worldTransferService)
	, _worldAdmissionService(worldAdmissionService)
	, _persenceManager(persenceManager)
{
}

WorldRuntime* ExecutionOps::GetRuntime(
	ExecScopeId scopeId, 
	std::span<WorldRuntime*> runtimeByScope) noexcept
{
	if (scopeId == InvalidExecScopeId)
		return nullptr;

	if (scopeId >= runtimeByScope.size())
		return nullptr;

	return runtimeByScope[scopeId];
}

void ExecutionOps::CommitScope(
	ExecScopeId scopeId, 
	std::span<WorldRuntime*> runtimeByScope)
{
	WorldRuntime* runtime = GetRuntime(scopeId, runtimeByScope);
	if (runtime == nullptr)
		throw std::runtime_error("ExecutionOps::CommitScope - runtime not found for scope");

	if (!runtime->FlushFrameCommands())
	{
		throw std::runtime_error("ExecutionOps::CommitScope - FlushFrameCommands failed");
	}
}

void ExecutionOps::FlushLifecycle(
	ExecScopeId scopeId, 
	std::span<WorldRuntime*> runtimeByScope)
{
	WorldRuntime* runtime = GetRuntime(scopeId, runtimeByScope);
	if (runtime == nullptr)
		throw std::runtime_error("ExecutionOps::FlushLifecycle - runtime not found for scope");

	if (!runtime->FlushLifecycleCommands())
	{
		throw std::runtime_error("ExecutionOps::FlushLifecycle - FlushLifecycleCommands failed");
	}
}

void ExecutionOps::ReconcileScope(
	ExecScopeId scopeId, 
	std::span<WorldRuntime*> runtimeByScope)
{
	WorldRuntime* runtime = GetRuntime(scopeId, runtimeByScope);
	if (runtime == nullptr)
		throw std::runtime_error("ExecutionOps::ReconcileScope - runtime not found for scope");

	// Lifecycle outbox is harvested by FrameworkRuntime after RunFrame returns.
	// Reconcile stays as the ordering boundary only.
	(void)runtime;
}

