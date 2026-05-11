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
	SessionFlowController& sessionFlow)
{
	WorldTransferEventBatch events{};
	framework.DrainWorldTransferEvents(events);

	return Commit(framework, sessionFlow, events);
}

bool ServerWorldTransferCommitter::Commit(
	FrameworkRuntime& framework,
	SessionFlowController& sessionFlow,
	const WorldTransferEventBatch& events)
{
	for (const WorldTransferFailedEvent& failed : events.failed)
	{
		HandleFailed(failed);
	}

	for (const WorldTransferCompletedEvent& completed : events.completed)
	{
		if (!CommitCompleted(framework, sessionFlow, completed))
		{
			return false;
		}
	}

	return true;
}

bool ServerWorldTransferCommitter::CommitCompleted(
	FrameworkRuntime& framework,
	SessionFlowController& sessionFlow,
	const WorldTransferCompletedEvent& event)
{
	if (!ValidateCompleted(framework, sessionFlow, event))
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
			FWLOG_ERROR(kLogCategory,
				"Commit failed: BindNetEntity (transferId=%u, sid=%u, netId=%u, targetWorldId=%u, targetEntity=%u.%u)",
				event.transferId, imported.sessionId, imported.netId.GetRaw(),
				event.targetWorldId.GetRaw(),
				imported.targetEntity.id, imported.targetEntity.generation);
			return false;
		}

		if (!sessionFlow.BindPlayer(
			imported.sessionId,
			imported.netId,
			event.targetWorldId))
		{
			FWLOG_ERROR(kLogCategory,
				"Commit failed: session bind (transferId=%u, sid=%u, netId=%u, targetWorldId=%u)",
				event.transferId, imported.sessionId,
				imported.netId.GetRaw(), event.targetWorldId.GetRaw());
			return false;
		}

		// 바인딩 검증: SessionFlow에서 직접 확인
		const NetId  committedNetId   = sessionFlow.FindControlledNetId(imported.sessionId);
		const WorldId committedWorldId = sessionFlow.FindCurrentWorldId(imported.sessionId);
		if (committedNetId != imported.netId || committedWorldId != event.targetWorldId)
		{
			FWLOG_ERROR(kLogCategory,
				"Commit failed: session binding verification "
				"(transferId=%u, sid=%u, expectedNetId=%u, expectedWorldId=%u, actualNetId=%u, actualWorldId=%u)",
				event.transferId, imported.sessionId,
				imported.netId.GetRaw(), event.targetWorldId.GetRaw(),
				committedNetId.GetRaw(), committedWorldId.GetRaw());
			return false;
		}

		const NetId sourceNetId =
			framework.FindNetId(event.sourceWorldId, imported.sourceEntity);
		const NetId targetNetId =
			framework.FindNetId(event.targetWorldId, imported.targetEntity);
		if (sourceNetId.IsValid() || targetNetId != imported.netId)
		{
			FWLOG_ERROR(kLogCategory,
				"Commit failed: net binding verification (transferId=%u, sid=%u, expectedNetId=%u, sourceNetId=%u, targetNetId=%u)",
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
	FWLOG_WARN(kLogCategory,
		"Transfer failed (transferId=%u, sourceWorldId=%u, resolvedTargetWorldId=%u, "
		"failedStage=%d, reason=%d, rollbackRequired=%d, retryCount=%u)",
		event.transferId,
		event.sourceWorldId.GetRaw(),
		event.resolvedTargetWorldId.GetRaw(),
		static_cast<int>(event.failedStage),
		static_cast<int>(event.reason),
		event.rollbackRequired ? 1 : 0,
		event.retryCount);
}

bool ServerWorldTransferCommitter::ValidateCompleted(
	const FrameworkRuntime& framework,
	const SessionFlowController& sessionFlow,
	const WorldTransferCompletedEvent& event)
{
	if (!event.sourceWorldId.IsValid() || !event.targetWorldId.IsValid())
	{
		FWLOG_ERROR(kLogCategory,
			"Validation failed: invalid world id (transferId=%u, sourceWorldId=%u, targetWorldId=%u)",
			event.transferId,
			event.sourceWorldId.GetRaw(),
			event.targetWorldId.GetRaw());
		return false;
	}

	if (event.sessionIds.empty() ||
		event.sessionIds.size() != event.importedEntities.size() ||
		event.sessionIds.size() != event.releasedSessionIds.size())
	{
		FWLOG_ERROR(kLogCategory,
			"Validation failed: count mismatch (transferId=%u, sessions=%zu, imported=%zu, released=%zu)",
			event.transferId,
			event.sessionIds.size(),
			event.importedEntities.size(),
			event.releasedSessionIds.size());
		return false;
	}

	for (const ImportedTransferEntity& imported : event.importedEntities)
	{
		if (imported.sessionId == 0 ||
			imported.sourceEntity.IsNull() ||
			imported.targetEntity.IsNull() ||
			!imported.netId.IsValid())
		{
			FWLOG_ERROR(kLogCategory,
				"Validation failed: invalid imported entity (transferId=%u, sid=%u, netId=%u)",
				event.transferId, imported.sessionId, imported.netId.GetRaw());
			return false;
		}

		if (std::find(
			event.sessionIds.begin(),
			event.sessionIds.end(),
			imported.sessionId) == event.sessionIds.end())
		{
			FWLOG_ERROR(kLogCategory,
				"Validation failed: imported session missing (transferId=%u, sid=%u)",
				event.transferId, imported.sessionId);
			return false;
		}

		if (std::find(
			event.releasedSessionIds.begin(),
			event.releasedSessionIds.end(),
			imported.sessionId) == event.releasedSessionIds.end())
		{
			FWLOG_ERROR(kLogCategory,
				"Validation failed: released session missing (transferId=%u, sid=%u)",
				event.transferId, imported.sessionId);
			return false;
		}

		// SessionFlow에서 현재 바인딩 검증
		const NetId  bindingNetId   = sessionFlow.FindControlledNetId(imported.sessionId);
		const WorldId bindingWorldId = sessionFlow.FindCurrentWorldId(imported.sessionId);
		if (!bindingNetId.IsValid())
		{
			FWLOG_ERROR(kLogCategory,
				"Validation failed: missing session binding (transferId=%u, sid=%u)",
				event.transferId, imported.sessionId);
			return false;
		}

		if (bindingNetId != imported.netId || bindingWorldId != event.sourceWorldId)
		{
			FWLOG_ERROR(kLogCategory,
				"Validation failed: stale session binding "
				"(transferId=%u, sid=%u, bindingNetId=%u, eventNetId=%u, bindingWorldId=%u, sourceWorldId=%u)",
				event.transferId, imported.sessionId,
				bindingNetId.GetRaw(), imported.netId.GetRaw(),
				bindingWorldId.GetRaw(), event.sourceWorldId.GetRaw());
			return false;
		}

		const NetId targetNetId =
			framework.FindNetId(event.targetWorldId, imported.targetEntity);
		if (targetNetId.IsValid() && targetNetId != imported.netId)
		{
			FWLOG_ERROR(kLogCategory,
				"Validation failed: target already bound (transferId=%u, sid=%u, targetNetId=%u, eventNetId=%u)",
				event.transferId, imported.sessionId,
				targetNetId.GetRaw(), imported.netId.GetRaw());
			return false;
		}
	}

	return true;
}
