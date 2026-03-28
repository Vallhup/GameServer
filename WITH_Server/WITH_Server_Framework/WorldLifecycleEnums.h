#pragma once

#include <cstdint>

enum class WorldStage : uint8_t
{
	Allocated,			// registry에는 생성되었지만 아직 runtime 시작 전
	Bootstrapping,		// init / startup 중
	Running,			// admission / transfer 대상 가능
	Closing,			// 신규 admission 금지, 기존 플레이어 / 전이 정리 대기
	DestroyPending,		// destroy 직전, 새 작업 금지
	Destroyed,			// record는 남아 있어도 registry에는 실체 없음
	Faulted				// 운영 실패 상태
};

enum class PresenceStage : uint8_t
{
	None,
	Idle,				// 어떤 월드에도 아직 확정 귀속되지 않음
	Active,				// source world에 정상적으로 존재 중
	TransferPending,	// source -> target 전이 중
	ImportedToTarget,	// target import는 되었으나 source release 전
	Disconnected,		// 연결 끊김으로 presence 유지 중
	Removed,			// 완전히 제거됨
	Failed				// 복구 불가 오류 상태
};

enum class TransferStage : uint8_t
{
	Requested,				// request만 접수됨
	SourceValidated,		// source world / source membership 검증 완료
	TargetResolved,			// target world 확정
	AdmissionReserved,		// target slot reservation 완료
	TransferContextBuilt,   // source snapshot 확보 완료
	TargetImported,			// target 쪽 import/spawn 완료
	SourceReleased,			// source 쪽 release/despawn 완료
	Completed,				// 전이 완료
	Failed					// 실패 종료
};

enum class AdmissionDecision : uint8_t
{
	Accepted,
	RejectedStage,
	RejectedJoinPolicy,
	RejectedCapacity,
	RejectedReEntry,
	RejectedClosing,
	RejectedTransferOverflow
};

enum class TransferFailureReason : uint8_t
{
	None,
	TimedOut,
	StaleSource,
	TargetResolveFailed,
	AdmissionRejected,
	TransferContextBuildFailed,
	ImportFailed,
	SourceReleaseFailed,
	RollbackFailed
};

inline bool IsWorldRunnable(WorldStage stage)
{
	return stage == WorldStage::Running;
}

inline bool IsWorldTerminal(WorldStage stage)
{
	return
		stage == WorldStage::Destroyed ||
		stage == WorldStage::Faulted;
}

inline bool IsTransferTerminal(TransferStage stage)
{
	return
		stage == TransferStage::Completed ||
		stage == TransferStage::Failed;
}