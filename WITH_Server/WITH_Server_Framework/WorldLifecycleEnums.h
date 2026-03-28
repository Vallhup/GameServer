#pragma once

#include <cstdint>

enum class WorldStage : uint8_t
{
	Allocated,
	Bootstrapping,
	Running,
	Closing,
	DestroyPending,
	Destroyed,
	Faulted
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
	Requested,          // request만 접수됨
	SourceValidated,    // source world / source membership 검증 완료
	TargetResolved,     // target world 확정
	AdmissionReserved,  // target slot reservation 완료
	SnapshotBuilt,      // source snapshot 확보 완료
	TargetImported,     // target 쪽 import/spawn 완료
	SourceReleased,     // source 쪽 release/despawn 완료
	Completed,          // 전이 완료
	Failed              // 실패 종료
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
	StaleSource,
	TargetResolveFailed,
	AdmissionRejected,
	SnapshotFailed,
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