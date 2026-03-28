#pragma once

#include <unordered_map>

#include "PresenceRecord.h"

class PresenceManager final {
public:
	PresenceRecord* FindByConnectionId(uint32_t connectionId);
	const PresenceRecord* FindByConnectionId(uint32_t connectionId) const;

	PresenceRecord* EnsurePresence(uint32_t connectionId);

	bool AttachToWorld(
		uint32_t connectionId,
		WorldId worldId,
		const double nowSec
	);

	bool BeginTransfer(
		uint32_t connectionId,
		TransferId transferId,
		WorldId sourceWorldId,
		WorldId targetWorldId,
		const double nowSec
	);

	bool MarkTargetImported(
		uint32_t connectionId,
		TransferId transferId,
		WorldId targetWorldId,
		const double nowSec
	);

	bool CompleteTransfer(
		uint32_t connectionId,
		TransferId transferId,
		WorldId sourceWorldId,
		WorldId targetWorldId,
		const double nowSec
	);

	bool FailTransfer(
		uint32_t connectionId,
		TransferId transferId,
		const double nowSec
	);


	bool RemovePresence(uint32_t connectionId, const double nowSec);
	bool SetDisconnected(
		uint32_t connectionId, 
		bool disconnected, 
		const double nowSec
	);

private:
	std::unordered_map<uint32_t, PresenceRecord> _records;
	PresenceId _nextPresenceId{ 1 };
};