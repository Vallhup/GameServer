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
	JoinRequested,
	AdmissionReserved,
	Spawning,
	Active,
	Leaving,
	Snapshotting,
	Importing,
	DespawnQueued,
	Removed,
	Failed
};

enum class TransferStage : uint8_t
{
	Requested,
	SourceValidated,
	TargetResolved,
	AdmissionReserved,
	SnapshotBuilt,
	TargetImported,
	SourceReleased,
	Completed,
	Failed
};

enum class WorldLifetimeKind : uint8_t
{
	Persistent,
	Instanced,
	SessionScoped
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