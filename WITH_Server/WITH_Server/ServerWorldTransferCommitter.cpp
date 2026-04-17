#include "pch.h"
#include "ServerWorldTransferCommitter.h"

#include <algorithm>
#include <iostream>

bool ServerWorldTransferCommitter::Commit(
	FrameworkRuntime& framework,
	SessionBindingRegistry& sessionBindings)
{
	WorldTransferEventBatch events{};
	framework.DrainWorldTransferEvents(events);

	for (const WorldTransferFailedEvent& failed : events.failed)
	{
		HandleFailed(failed);
	}

	for (const WorldTransferCompletedEvent& completed : events.completed)
	{
		if (!CommitCompleted(framework, sessionBindings, completed))
		{
			return false;
		}
	}

	return true;
}

bool ServerWorldTransferCommitter::CommitCompleted(
	FrameworkRuntime& framework,
	SessionBindingRegistry& sessionBindings,
	const WorldTransferCompletedEvent& event)
{
	if (!ValidateCompleted(framework, sessionBindings, event))
	{
		return false;
	}

	for (const ImportedTransferEntity& imported : event.importedEntities)
	{
		if (!framework.BindNetEntity(
			imported.netId,
			event.targetWorldId,
			imported.targetEntity))
		{
			std::cout << "[WorldTransfer] completed commit failed: BindNetEntity"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " netId=" << imported.netId.GetRaw()
				<< " targetWorldId=" << event.targetWorldId.GetRaw()
				<< " targetEntity=(" << imported.targetEntity.id
				<< "," << imported.targetEntity.generation << ")"
				<< "\n";
			return false;
		}

		if (!sessionBindings.Bind(
			imported.sessionId,
			imported.netId,
			event.targetWorldId))
		{
			std::cout << "[WorldTransfer] completed commit failed: session bind"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " netId=" << imported.netId.GetRaw()
				<< " targetWorldId=" << event.targetWorldId.GetRaw()
				<< "\n";
			return false;
		}

		const SessionBinding* committedBinding =
			sessionBindings.FindBySession(imported.sessionId);
		if (committedBinding == nullptr ||
			committedBinding->controlledNetId != imported.netId ||
			committedBinding->currentWorldId != event.targetWorldId)
		{
			std::cout << "[WorldTransfer] completed commit failed: session binding verification"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " expectedNetId=" << imported.netId.GetRaw()
				<< " expectedWorldId=" << event.targetWorldId.GetRaw();

			if (committedBinding != nullptr)
			{
				std::cout
					<< " actualNetId=" << committedBinding->controlledNetId.GetRaw()
					<< " actualWorldId=" << committedBinding->currentWorldId.GetRaw();
			}

			std::cout << "\n";
			return false;
		}

		const NetId sourceNetId =
			framework.FindNetId(event.sourceWorldId, imported.sourceEntity);
		const NetId targetNetId =
			framework.FindNetId(event.targetWorldId, imported.targetEntity);
		if (sourceNetId.IsValid() || targetNetId != imported.netId)
		{
			std::cout << "[WorldTransfer] completed commit failed: net binding verification"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " expectedNetId=" << imported.netId.GetRaw()
				<< " sourceNetId=" << sourceNetId.GetRaw()
				<< " targetNetId=" << targetNetId.GetRaw()
				<< "\n";
			return false;
		}
	}

	return true;
}

void ServerWorldTransferCommitter::HandleFailed(
	const WorldTransferFailedEvent& event)
{
	std::cout << "[WorldTransfer] failed"
		<< " transferId=" << event.transferId
		<< " sourceWorldId=" << event.sourceWorldId.GetRaw()
		<< " resolvedTargetWorldId=" << event.resolvedTargetWorldId.GetRaw()
		<< " failedStage=" << static_cast<int>(event.failedStage)
		<< " reason=" << static_cast<int>(event.reason)
		<< " rollbackRequired=" << event.rollbackRequired
		<< " retryCount=" << event.retryCount
		<< "\n";
}

bool ServerWorldTransferCommitter::ValidateCompleted(
	const FrameworkRuntime& framework,
	const SessionBindingRegistry& sessionBindings,
	const WorldTransferCompletedEvent& event)
{
	if (!event.sourceWorldId.IsValid() || !event.targetWorldId.IsValid())
	{
		std::cout << "[WorldTransfer] completed validation failed: invalid world id"
			<< " transferId=" << event.transferId
			<< " sourceWorldId=" << event.sourceWorldId.GetRaw()
			<< " targetWorldId=" << event.targetWorldId.GetRaw()
			<< "\n";
		return false;
	}

	if (event.sessionIds.empty() ||
		event.sessionIds.size() != event.importedEntities.size() ||
		event.sessionIds.size() != event.releasedSessionIds.size())
	{
		std::cout << "[WorldTransfer] completed validation failed: count mismatch"
			<< " transferId=" << event.transferId
			<< " sessions=" << event.sessionIds.size()
			<< " imported=" << event.importedEntities.size()
			<< " released=" << event.releasedSessionIds.size()
			<< "\n";
		return false;
	}

	for (const ImportedTransferEntity& imported : event.importedEntities)
	{
		if (imported.sessionId == 0 ||
			imported.sourceEntity.IsNull() ||
			imported.targetEntity.IsNull() ||
			!imported.netId.IsValid())
		{
			std::cout << "[WorldTransfer] completed validation failed: invalid imported entity"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " netId=" << imported.netId.GetRaw()
				<< "\n";
			return false;
		}

		if (std::find(
			event.sessionIds.begin(),
			event.sessionIds.end(),
			imported.sessionId) == event.sessionIds.end())
		{
			std::cout << "[WorldTransfer] completed validation failed: imported session missing"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< "\n";
			return false;
		}

		if (std::find(
			event.releasedSessionIds.begin(),
			event.releasedSessionIds.end(),
			imported.sessionId) == event.releasedSessionIds.end())
		{
			std::cout << "[WorldTransfer] completed validation failed: released session missing"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< "\n";
			return false;
		}

		const SessionBinding* binding =
			sessionBindings.FindBySession(imported.sessionId);
		if (binding == nullptr)
		{
			std::cout << "[WorldTransfer] completed validation failed: missing session binding"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< "\n";
			return false;
		}

		if (binding->controlledNetId != imported.netId ||
			binding->currentWorldId != event.sourceWorldId)
		{
			std::cout << "[WorldTransfer] completed validation failed: stale session binding"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " bindingNetId=" << binding->controlledNetId.GetRaw()
				<< " eventNetId=" << imported.netId.GetRaw()
				<< " bindingWorldId=" << binding->currentWorldId.GetRaw()
				<< " sourceWorldId=" << event.sourceWorldId.GetRaw()
				<< "\n";
			return false;
		}

		const NetId targetNetId =
			framework.FindNetId(event.targetWorldId, imported.targetEntity);
		if (targetNetId.IsValid() && targetNetId != imported.netId)
		{
			std::cout << "[WorldTransfer] completed validation failed: target already bound"
				<< " transferId=" << event.transferId
				<< " sessionId=" << imported.sessionId
				<< " targetNetId=" << targetNetId.GetRaw()
				<< " eventNetId=" << imported.netId.GetRaw()
				<< "\n";
			return false;
		}
	}

	return true;
}
