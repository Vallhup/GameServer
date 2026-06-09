#pragma once

#include <cstdint>

#include "Session.h"
#include "WorldIds.h"

class IWorldTransitionRequestSink {
public:
	virtual ~IWorldTransitionRequestSink() = default;

	virtual TransferId RequestDemoWorldTransition(
		SessionId sessionId,
		uint32_t requestId) = 0;

	virtual bool MarkClientWorldTransitionReady(
		SessionId sessionId,
		TransferId transferId) = 0;

	// FinalBoss 처치 후 파티 선택 투표(SC_FINAL_CLEAR_CHOICE_BEGIN)에 대한
	// 클라이언트 응답(CS_FINAL_CLEAR_CHOICE_SUBMIT)을 처리한다.
	virtual bool SubmitFinalClearPvpChoice(
		SessionId sessionId,
		uint64_t voteId,
		bool choosePvp) = 0;

	virtual void OnSessionDisconnected(SessionId sessionId) noexcept = 0;
};
