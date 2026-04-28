#include "pch.h"
#include "ServerWorldTransferCommitter.h"

#include <algorithm>

#include "FrameworkLog.h"

namespace
{
	constexpr const char* kLogCategory = "WorldTransfer";
}

bool ServerWorldTransferCommitter::Commit(
	FrameworkRuntime& framework,
	SessionBindingRegistry& sessionBindings)
{
	WorldTransferEventBatch events{};
	framework.DrainWorldTransferEvents(events);

	return Commit(framework, sessionBindings, events);
}

bool ServerWorldTransferCommitter::Commit(
	FrameworkRuntime& framework,
	SessionBindingRegistry& sessionBindings,
	const WorldTransferEventBatch& events)
{
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
			FWLOG_ERROR(kLogCategory, "Commit failed: BindNetEntity (transferId=%u, sid=%u, netId=%u, targetWorldId=%u, targetEntity=%u.%u)",
				event.transferId, imported.sessionId, imported.netId.GetRaw(),
				event.targetWorldId.GetRaw(), imported.targetEntity.id, imported.targetEntity.generation);
			return false;
		}

		if (!sessionBindings.Bind(
			imported.sessionId,
			imported.netId,
			event.targetWorldId))
		{
			FWLOG_ERROR(kLogCategory, "Commit failed: session bind (transferId=%u, sid=%u, netId=%u, targetWorldId=%u)",
				event.transferId, imported.sessionId, imported.netId.GetRaw(), event.targetWorldId.GetRaw());
			return false;
		}

		const SessionBinding* committedBinding =
			sessionBindings.FindBySession(imported.sessionId);
		if (committedBinding == nullptr ||
			committedBinding->controlledNetId != imported.netId ||
			committedBinding->currentWorldId != event.targetWorldId)
		{
			if (committedBinding != nullptr)
			{
				FWLOG_ERROR(kLogCategory, "Commit failed: session binding verification (transferId=%u, sid=%u, expectedNetId=%u, expectedWorldId=%u, actualNetId=%u, actualWorldId=%u)",
					event.transferId, imported.sessionId,
					imported.netId.GetRaw(), event.targetWorldId.GetRaw(),
					committedBinding->controlledNetId.GetRaw(), committedBinding->currentWorldId.GetRaw());
			}
			else
			{
				FWLOG_ERROR(kLogCategory, "Commit failed: session binding verification (transferId=%u, sid=%u, expectedNetId=%u, expectedWorldId=%u, binding=null)",
					event.transferId, imported.sessionId,
					imported.netId.GetRaw(), event.targetWorldId.GetRaw());
			}
			return false;
		}

		const NetId sourceNetId =
			framework.FindNetId(event.sourceWorldId, imported.sourceEntity);
		const NetId targetNetId =
			framework.FindNetId(event.targetWorldId, imported.targetEntity);
		if (sourceNetId.IsValid() || targetNetId != imported.netId)
		{
			FWLOG_ERROR(kLogCategory, "Commit failed: net binding verification (transferId=%u, sid=%u, expectedNetId=%u, sourceNetId=%u, targetNetId=%u)",
				event.transferId, imported.sessionId,
				imported.netId.GetRaw(), sourceNetId.GetRaw(), targetNetId.GetRaw());
			return false;
		}
	}

	return true;
}

void ServerWorldTransferCommitter::HandleFailed(
	const WorldTransferFailedEvent& event)
{
	FWLOG_WARN(kLogCategory, "Transfer failed (transferId=%u, sourceWorldId=%u, resolvedTargetWorldId=%u, failedStage=%d, reason=%d, rollbackRequired=%d, retryCount=%u)",
		event.transferId, event.sourceWorldId.GetRaw(), event.resolvedTargetWorldId.GetRaw(),
		static_cast<int>(event.failedStage), static_cast<int>(event.reason),
		event.rollbackRequired ? 1 : 0, event.retryCount);
}

bool ServerWorldTransferCommitter::ValidateCompleted(
	const FrameworkRuntime& framework,
	const SessionBindingRegistry& sessionBindings,
	const WorldTransferCompletedEvent& event)
{
	if (!event.sourceWorldId.IsValid() || !event.targetWorldId.IsValid())
	{
		FWLOG_ERROR(kLogCategory, "Validation failed: invalid world id (transferId=%u, sourceWorldId=%u, targetWorldId=%u)",
			event.transferId, event.sourceWorldId.GetRaw(), event.targetWorldId.GetRaw());
		return false;
	}

	if (event.sessionIds.empty() ||
		event.sessionIds.size() != event.importedEntities.size() ||
		event.sessionIds.size() != event.releasedSessionIds.size())
	{
		FWLOG_ERROR(kLogCategory, "Validation failed: count mismatch (transferId=%u, sessions=%zu, imported=%zu, released=%zu)",
			event.transferId, event.sessionIds.size(), event.importedEntities.size(), event.releasedSessionIds.size());
		return false;
	}

	for (const ImportedTransferEntity& imported : event.importedEntities)
	{
		if (imported.sessionId == 0 ||
			imported.sourceEntity.IsNull() ||
			imported.targetEntity.IsNull() ||
			!imported.netId.IsValid())
		{
			FWLOG_ERROR(kLogCategory, "Validation failed: invalid imported entity (transferId=%u, sid=%u, netId=%u)",
				event.transferId, imported.sessionId, imported.netId.GetRaw());
			return false;
		}

		if (std::find(
			event.sessionIds.begin(),
			event.sessionIds.end(),
			imported.sessionId) == event.sessionIds.end())
		{
			FWLOG_ERROR(kLogCategory, "Validation failed: imported session missing (transferId=%u, sid=%u)",
				event.transferId, imported.sessionId);
			return false;
		}

		if (std::find(
			event.releasedSessionIds.begin(),
			event.releasedSessionIds.end(),
			imported.sessionId) == event.releasedSessionIds.end())
		{
			FWLOG_ERROR(kLogCategory, "Validation failed: released session missing (transferId=%u, sid=%u)",
				event.transferId, imported.sessionId);
			return false;
		}

		const SessionBinding* binding =
			sessionBindings.FindBySession(imported.sessionId);
		if (binding == nullptr)
		{
			FWLOG_ERROR(kLogCategory, "Validation failed: missing session binding (transferId=%u, sid=%u)",
				event.transferId, imported.sessionId);
			return false;
		}

		if (binding->controlledNetId != imported.netId ||
			binding->currentWorldId != event.sourceWorldId)
		{
			FWLOG_ERROR(kLogCategory, "Validation failed: stale session binding (transferId=%u, sid=%u, bindingNetId=%u, eventNetId=%u, bindingWorldId=%u, sourceWorldId=%u)",
				event.transferId, imported.sessionId,
				binding->controlledNetId.GetRaw(), imported.netId.GetRaw(),
				binding->currentWorldId.GetRaw(), event.sourceWorldId.GetRaw());
			return false;
		}

		const NetId targetNetId =
			framework.FindNetId(event.targetWorldId, imported.targetEntity);
		if (targetNetId.IsValid() && targetNetId != imported.netId)
		{
			FWLOG_ERROR(kLogCategory, "Validation failed: target already bound (transferId=%u, sid=%u, targetNetId=%u, eventNetId=%u)",
				event.transferId, imported.sessionId, targetNetId.GetRaw(), imported.netId.GetRaw());
			return false;
		}
	}

	return true;
}
