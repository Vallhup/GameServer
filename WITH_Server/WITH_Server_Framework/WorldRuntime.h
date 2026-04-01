#pragma once

#include <memory>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "WorldMutationBuffer.h"
#include "WorldRuntimeTypes.h"
#include "ECSCore.h"
#include "ECSView.h"
#include "WorldLifecycleBuffer.h"
#include "SystemManager.h"
#include "Entity.h"
#include "Component.h"

class ITransferContext;
class IWorldTransferBinding;
class WorldTransferProfile;

class WorldRuntime final {
public:
	explicit WorldRuntime(WorldRuntimeCreateParams params = {});
	~WorldRuntime() = default;

	WorldRuntime(const WorldRuntime&) = delete;
	WorldRuntime& operator=(const WorldRuntime&) = delete;
	WorldRuntime(WorldRuntime&&) noexcept = default;
	WorldRuntime& operator=(WorldRuntime&&) noexcept = default;

public:
	bool Initialize();
	void Shutdown();

	// execution layer only
	bool BeginFrame(uint64_t frameIndex, double nowSec, double dtSec);

	bool FlushFrameCommands();
	bool FlushLifecycleCommands();

	bool BuildTransferContext(
		const std::vector<uint32_t>& sessionIds,
		std::unique_ptr<ITransferContext>& outContext);

	bool ImportTransferContext(
		const ITransferContext& context,
		std::vector<uint32_t>& outImportedSessionIds);

	bool ReleaseTransferContext(
		const ITransferContext& context,
		std::vector<uint32_t>& outReleasedSessionIds);

	bool RollbackImportedTransferContext(
		const ITransferContext& context,
		const std::vector<uint32_t>& importedSessionIds);

public:
	const WorldDef* GetDef() const noexcept { return _def; }
	const WorldExecutionModel* GetExecutionModel() const noexcept { return _executionModel; }
	const IWorldTransferBinding* GetTransferBinding() const noexcept { return _transferBinding; }
	const WorldTransferProfile* GetTransferProfile() const noexcept { return _transferProfile; }

	WorldRuntimeLifecycleState GetLifecycleState() const noexcept { return _lifecycleState; }
	WorldRuntimeCommitState GetCommitState() const noexcept { return _commitState; }
	WorldRuntimeLifecycleFlushState GetLifecycleFlushState() const noexcept { return _lifecycleFlushState; }

	bool IsInitialized() const noexcept
	{
		return _lifecycleState == WorldRuntimeLifecycleState::Running;
	}

	bool IsShutdown() const noexcept
	{
		return _lifecycleState == WorldRuntimeLifecycleState::Shutdown;
	}

	bool IsFaulted() const noexcept { return _fault.HasError(); }

	uint64_t FrameIndex() const noexcept { return _frameIndex; }
	double LastNowSec() const noexcept { return _lastNowSec; }
	double LastDtSec() const noexcept { return _lastDtSec; }

	const WorldRuntimeFault& GetFault() const noexcept { return _fault; }

	ECSView MakeView() noexcept
	{
		return ECSView(_ecs);
	}

	ECSView MakeView() const noexcept
	{
		return ECSView(_ecs);
	}

	std::span<const WorldLifecycleCommand> LifecycleOutbox() const noexcept
	{
		return std::span<const WorldLifecycleCommand>(
			_lifecycleOutbox.data(),
			_lifecycleOutbox.size());
	}

	void ClearLifecycleOutbox() noexcept
	{
		_lifecycleOutbox.clear();
	}

public:
	template<CompT T>
	void RegisterStorage()
	{
		EnsureStorageRegistrationAllowed();
		_ecs.RegisterStorage<T>();
	}

	void FixStorages()
	{
		_ecs.FixStorages();
		_storagesFixed = true;
	}

	template<SysT T, typename... Args>
	T* RegisterSystem(SystemPhase phase, Args&&... args)
	{
		return _systems.RegisterSystem<T>(phase, std::forward<Args>(args)...);
	}

public:
	Entity ReserveEntity();

	void DeferredDestroyEntity(Entity e)
	{
		if (!CanAcceptStructuralMutation())
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"DeferredDestroyEntity is not allowed in the current runtime state.");
			return;
		}

		_frameCommands.Enqueue(
			[entity = e](WorldRuntime& rt)
			{
				(void)rt.DestroyEntityImmediate(entity);
			});
	}

	template<CompT T, typename... Args>
	void DeferredAddComponent(Entity e, Args&&... args)
	{
		if (!CanAcceptStructuralMutation())
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"DeferredAddComponent is not allowed in the current runtime state.");
			return;
		}

		auto payload = std::make_tuple(std::forward<Args>(args)...);
		_frameCommands.Enqueue(
			[entity = e, payload = std::move(payload)](WorldRuntime& rt) mutable
			{
				std::apply(
					[&](auto&&... unpacked)
					{
						(void)rt.AddComponentImmediate<T>(
							entity,
							std::forward<decltype(unpacked)>(unpacked)...);
					},
					std::move(payload));
			});
	}

	template<CompT T>
	void DeferredUpsertComponent(Entity e, T value)
	{
		if (!CanAcceptStructuralMutation())
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"DeferredUpsertComponent is not allowed in the current runtime state.");
			return;
		}

		_frameCommands.Enqueue(
			[entity = e, component = std::move(value)](WorldRuntime& rt) mutable
			{
				(void)rt.AddOrAssignComponentImmediate<T>(entity, std::move(component));
			});
	}

	template<CompT T>
	void DeferredRemoveComponent(Entity e)
	{
		if (!CanAcceptStructuralMutation())
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"DeferredRemoveComponent is not allowed in the current runtime state.");
			return;
		}

		_frameCommands.Enqueue(
			[entity = e](WorldRuntime& rt)
			{
				(void)rt.RemoveComponentImmediate<T>(entity);
			});
	}

	void EnqueueLifecycle(const WorldLifecycleCommand& command)
	{
		if (!CanAcceptLifecycleSignal())
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"EnqueueLifecycle is not allowed in the current runtime state.");
			return;
		}

		_lifecycleCommands.Enqueue(command);
	}

	void EnqueueLifecycle(WorldLifecycleCommand&& command)
	{
		if (!CanAcceptLifecycleSignal())
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"EnqueueLifecycle is not allowed in the current runtime state.");
			return;
		}

		_lifecycleCommands.Enqueue(std::move(command));
	}

private:
	void EnsureStorageRegistrationAllowed() const;

	void MarkFault(WorldRuntimeFaultCode code, const char* message);

	bool CanBeginFrame() const;
	bool CanFlushFrameCommands() const;
	bool CanFlushLifecycleCommands() const;
	bool CanAcceptStructuralMutation() const;
	bool CanAcceptLifecycleSignal() const;

	bool MaterializeReservedEntityImmediate(Entity reserved);
	bool DestroyEntityImmediate(Entity e);

	template<CompT T, typename... Args>
	T* AddComponentImmediate(Entity e, Args&&... args)
	{
		auto* storage = _ecs.TryGetStorage<T>();
		if (!storage)
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"AddComponentImmediate called for unregistered storage.");
			return nullptr;
		}

		return _ecs.AddComponentImmediate<T>(e, std::forward<Args>(args)...);
	}

	template<CompT T>
	T* AddOrAssignComponentImmediate(Entity e, T value)
	{
		auto* storage = _ecs.TryGetStorage<T>();
		if (!storage)
		{
			MarkFault(
				WorldRuntimeFaultCode::InvalidOperation,
				"AddOrAssignComponentImmediate called for unregistered storage.");
			return nullptr;
		}

		return _ecs.AddOrAssignComponentImmediate<T>(e, std::move(value));
	}

	template<CompT T>
	bool RemoveComponentImmediate(Entity e)
	{
		auto* storage = _ecs.TryGetStorage<T>();
		if (!storage)
			return false;

		if (!storage->HasComponent(e))
			return false;

		return _ecs.RemoveComponentImmediate<T>(e);
	}

private:
	const WorldDef* _def{ nullptr };
	const WorldExecutionModel* _executionModel{ nullptr };
	const IWorldTransferBinding* _transferBinding{ nullptr };
	const WorldTransferProfile* _transferProfile{ nullptr };

	WorldRuntimeLifecycleState _lifecycleState{ WorldRuntimeLifecycleState::Constructed };
	WorldRuntimeCommitState _commitState{ WorldRuntimeCommitState::NotCommitted };
	WorldRuntimeLifecycleFlushState _lifecycleFlushState{ WorldRuntimeLifecycleFlushState::NotFlushed };

	WorldMutationBuffer _frameCommands;
	WorldLifecycleBuffer _lifecycleCommands;
	std::vector<WorldLifecycleCommand> _lifecycleOutbox;
	ECSCore _ecs;
	SystemManager _systems;

	bool _storagesFixed{ false };
	bool _hasBegunAnyFrame{ false };
	bool _frameOpen{ false };

	uint64_t _frameIndex{ 0 };
	double _lastNowSec{ 0.0 };
	double _lastDtSec{ 0.0 };

	WorldRuntimeFault _fault;
};
