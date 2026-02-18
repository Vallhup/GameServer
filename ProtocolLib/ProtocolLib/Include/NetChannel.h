#pragma once

#include <vector>
#include "SPSCBuffer.h"

struct InboxEvent {};
struct OutboxEvent {};

class NetChannel {
	static constexpr size_t kInboxCap{ 1u << 14 };
	static constexpr size_t kOutboxCap{ 1u << 14 };

public:
	NetChannel(size_t netThreadCnt)
		: _inboxes(netThreadCnt), _outboxes(netThreadCnt) {}

	bool TryPushInbox(uint32 threadId, const InboxEvent& ev)
	{ 
		return _inboxes[threadId].TryPush(ev); 
	}

	bool TryPushOutbox(uint32 threadId, const OutboxEvent& ev)
	{
		return _outboxes[threadId].TryPush(ev);
	}

	template<typename Fn>
	void DrainInbox(uint32 threadId, int budget, Fn&& fn)
	{
		int n{ 0 };
		InboxEvent ev;
		while (n < budget && _inboxes[threadId].TryPop(ev))
		{
			fn(ev);
			++n;
		}
	}

	template<typename Fn>
	void DrainOutbox(uint32 threadId, int budget, Fn&& fn)
	{
		int n{ 0 };
		OutboxEvent ev;
		while (n < budget && _outboxes[threadId].TryPop(ev))
		{
			fn(ev);
			++n;
		}
	}

private:
	std::vector<SPSCBuffer<InboxEvent, kInboxCap>> _inboxes;
	std::vector<SPSCBuffer<OutboxEvent, kOutboxCap>> _outboxes;
};

