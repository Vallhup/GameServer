#pragma once

#include <memory>
#include <unordered_map>

#include "SessionFlowCommands.h"
#include "SessionFlowContext.h"
#include "SessionFlowResults.h"
#include "SessionFlowTransitionTable.h"
#include "SessionState.h"

class SessionFlowController final {
public:
	explicit SessionFlowController(SessionFlowDependencies dependencies = {});

	void SetDependencies(SessionFlowDependencies dependencies) noexcept;
	void Clear() noexcept;

	void OnSessionConnected(SessionId sessionId);
	void OnSessionDisconnected(SessionId sessionId);

	SessionFlowResult Dispatch(SessionId sessionId, const ISessionCommand& command);

	bool CanAcceptGameplay(SessionId sessionId) const;
	bool CanAcceptReplicationAck(SessionId sessionId) const;

	SessionStateId GetState(SessionId sessionId) const;
	SessionFlow* FindFlow(SessionId sessionId) noexcept;
	const SessionFlow* FindFlow(SessionId sessionId) const noexcept;

private:
	struct Entry
	{
		SessionFlow flow;
		std::unique_ptr<ISessionState> state;
	};

	Entry* EnsureEntry(SessionId sessionId);
	void RegisterDefaultTransitions();
	SessionFlowResult ApplyTransition(Entry& entry, const TransitionResult& transition);

	std::unordered_map<SessionId, Entry> _sessions;
	SessionFlowTransitionTable _transitions;
	SessionFlowDependencies _dependencies;
};
