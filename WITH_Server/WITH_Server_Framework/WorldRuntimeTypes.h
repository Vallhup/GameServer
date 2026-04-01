#pragma once

#include <cstdint>
#include <string>

struct WorldDef;
struct WorldExecutionModel;
class WorldTransferProfile;
class IWorldTransferBinding;

enum class WorldRuntimeLifecycleState : uint8_t
{
	Constructed,
	Running,
	Shutdown
};

enum class WorldRuntimeCommitState : uint8_t
{
	NotCommitted,
	CommitSucceeded,
	CommitFailed
};

enum class WorldRuntimeLifecycleFlushState : uint8_t
{
	NotFlushed,
	Flushed
};

enum class WorldRuntimeFaultCode : uint8_t
{
	None,
	InitFailed,
	StorageRegistrationFailed,
	SystemRegistrationFailed,
	FrameExecuteFailed,
	TransferBuildFailed,
	TransferImportFailed,
	TransferReleaseFailed,
	TransferRollbackFailed,
	InvalidOperation
};

struct WorldRuntimeFault
{
	WorldRuntimeFaultCode code{ WorldRuntimeFaultCode::None };
	std::string message;

	bool HasError() const noexcept
	{
		return code != WorldRuntimeFaultCode::None;
	}
};

struct WorldRuntimeCreateParams
{
	const WorldDef* def{ nullptr };
	const WorldExecutionModel* executionModel{ nullptr };
	const WorldTransferProfile* transferProfile{ nullptr };
	const IWorldTransferBinding* transferBinding{ nullptr };
};
