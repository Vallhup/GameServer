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

	virtual bool RequestBeaconCinematicStart(
		SessionId sessionId,
		uint32_t clientRequestId) = 0;

	// FinalBoss 처치 후 파티 선택 투표(SC_FINAL_CLEAR_CHOICE_BEGIN)에 대한
	// 클라이언트 응답(CS_FINAL_CLEAR_CHOICE_SUBMIT)을 처리한다.
	virtual bool SubmitFinalClearPvpChoice(
		SessionId sessionId,
		uint64_t voteId,
		bool choosePvp) = 0;

	// 클라이언트 엔딩/페이드 연출 완료 통지(CS_FINAL_ENDING_CINEMATIC_DONE).
	// context: FinalEndingCinematicContext (1=FinalClear, 2=PvpRoundEnd).
	virtual bool SubmitFinalEndingCinematicDone(
		SessionId sessionId,
		uint32_t context) = 0;

	virtual void OnSessionDisconnected(SessionId sessionId) noexcept = 0;
};
