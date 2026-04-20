#pragma once
#include "../../ProtocolLib/ProtocolLib/Protocols/Protocol.pb.h"

enum class ClientWorldTransitionPhase : uint8_t
{
	Idle,
	RequestSent,
	BeginReceived,
	Loading,
	ReadySent,
	Rejected
};

struct ClientWorldTransitionContext
{
	uint32_t requestId{ 0 };
	uint64_t transferId{ 0 };

	uint32_t sourceWorldDefId{ 0 };
	uint64_t sourceWorldId{ 0 };

	uint32_t targetWorldDefId{ 0 };
	uint64_t targetWorldId{ 0 };

	uint32_t mapResourceId{ 0 };
	uint64_t playerNetId{ 0 };

	bool clearExistingObjects{ true };
	bool waitClientReady{ true };
	bool usedFallback{ false };

	uint32_t reason{ 0 };
};

class ClientWorldTransitionController {
public:
	uint32_t CreateRequestId();

	bool BeginRequest(uint32_t requestId);

	bool OnBegin(const Protocol::SC_WORLD_TRANSITION_BEGIN_PACKET& packet);
	bool OnRejected(const Protocol::SC_WORLD_TRANSITION_REJECTED_PACKET& packet);

	void MarkLoadingStarted();
	void MarkReadySent();
	void Reset();

	bool HasBeginContext() const;
	bool HasPendingReady() const;

	uint64_t GetTransferId() const { return context.transferId; }
	uint32_t GetTargetWorldDefId() const { return context.targetWorldDefId; }
	const ClientWorldTransitionContext& GetContext() const { return context; }

	ClientWorldTransitionPhase GetPhase() const { return phase; }

private:
	uint32_t nextRequestId{ 1 };
	ClientWorldTransitionPhase phase{ ClientWorldTransitionPhase::Idle };
	ClientWorldTransitionContext context;
};