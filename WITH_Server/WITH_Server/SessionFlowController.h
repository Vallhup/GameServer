#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

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

	SessionStateId     GetState(SessionId sessionId) const;
	SessionFlow*       FindFlow(SessionId sessionId) noexcept;
	const SessionFlow* FindFlow(SessionId sessionId) const noexcept;

	// ---------------------------------------------------------------
	// 바인딩 인덱스 조회 API (구 SessionBindingRegistry 공개 API 이식)
	// ---------------------------------------------------------------
	SessionId FindOwnerSession(NetId controlledNetId) const noexcept;
	SessionId FindOwnerSessionByAccountId(uint64_t accountId) const noexcept;
	bool      HasAccountLogin(uint64_t accountId) const noexcept;
	NetId     FindControlledNetId(SessionId sessionId) const noexcept;
	WorldId   FindCurrentWorldId(SessionId sessionId) const noexcept;
	bool      HasBinding(SessionId sessionId) const noexcept;
	void      CollectSessionsInWorld(
		WorldId worldId,
		std::vector<SessionId>& outSessionIds) const;

	// ---------------------------------------------------------------
	// 바인딩 변경 API
	// ---------------------------------------------------------------

	// transition 핸들러 외부에서 바인딩이 필요한 경우 사용
	// (예: ServerSessionSystem::MarkInitialWorldReady, ServerWorldTransferCommitter)
	// controlledNetId는 LoginSucceeded 시점에 이미 flow에 기록되어 있어야 한다.
	// BindPlayer는 worldId를 받아 currentWorldId를 설정하고 역인덱스를 갱신한다.
	bool BindPlayer(SessionId sessionId, WorldId currentWorldId) noexcept;

	// disconnect / 세션 정리 시 사용
	bool UnbindPlayer(SessionId sessionId) noexcept;

	// WorldTransfer commit 시 world 갱신
	bool UpdatePlayerWorld(SessionId sessionId, WorldId newWorldId) noexcept;

private:
	struct Entry
	{
		SessionFlow flow;
		std::unique_ptr<ISessionState> state;
	};

	Entry*            EnsureEntry(SessionId sessionId);
	void              RegisterDefaultTransitions();
	SessionFlowResult ApplyTransition(Entry& entry, const TransitionResult& transition);

	// 역인덱스 내부 갱신 헬퍼 (반드시 _indexMutex write lock 보유 상태에서 호출)
	void IndexBind_Locked(
		SessionId sessionId,
		NetId controlledNetId,
		WorldId currentWorldId);
	void IndexAccountBind_Locked(SessionId sessionId, uint64_t accountId);
	void IndexAccountUnbind_Locked(SessionId sessionId, uint64_t accountId) noexcept;
	void IndexUnbind_Locked(SessionId sessionId, NetId controlledNetId, WorldId currentWorldId) noexcept;
	void IndexUpdateWorld_Locked(SessionId sessionId, WorldId oldWorldId, WorldId newWorldId) noexcept;

	// 프레임 루프 단일 스레드 전용 — 무잠금
	std::unordered_map<SessionId, Entry> _sessions;
	SessionFlowTransitionTable           _transitions;
	SessionFlowDependencies              _dependencies;

	// 멀티스레드 read 허용 역인덱스 — _indexMutex 보호
	mutable std::shared_mutex                           _indexMutex;
	std::unordered_map<NetId, SessionId>                _sessionByNetId;
	std::unordered_map<uint64_t, SessionId>             _sessionByAccountId;
	std::unordered_map<WorldId, std::vector<SessionId>> _sessionsByWorld;
};
