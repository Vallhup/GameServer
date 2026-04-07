#pragma once

#include <unordered_map>

#include "PresenceRecord.h"

class PresenceManager final {
public:
	PresenceRecord* FindBySessionId(uint32_t sessionId);
	const PresenceRecord* FindBySessionId(uint32_t sessionId) const;

	PresenceRecord* EnsurePresence(uint32_t sessionId);

	bool AttachToWorld(
		uint32_t sessionId,
		WorldId worldId,
		const double nowSec
	);

	bool BeginTransfer(
		uint32_t sessionId,
		TransferId transferId,
		WorldId sourceWorldId,
		WorldId targetWorldId,
		const double nowSec
	);

	bool MarkTargetImported(
		uint32_t sessionId,
		TransferId transferId,
		WorldId targetWorldId,
		const double nowSec
	);

	bool CompleteTransfer(
		uint32_t sessionId,
		TransferId transferId,
		WorldId sourceWorldId,
		WorldId targetWorldId,
		const double nowSec
	);

	bool FailTransfer(
		uint32_t sessionId,
		TransferId transferId,
		const double nowSec
	);


	bool RemovePresence(uint32_t sessionId, const double nowSec);
	bool SetDisconnected(
		uint32_t sessionId, 
		bool disconnected, 
		const double nowSec
	);

	bool CanReEnterWorld(uint32_t sessionId, WorldId targetWorldId) const;

private:
	std::unordered_map<uint32_t, PresenceRecord> _records;
	PresenceId _nextPresenceId{ 1 };
};