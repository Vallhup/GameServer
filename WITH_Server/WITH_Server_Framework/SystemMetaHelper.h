#pragma once

#include "SystemMeta.h"

template<typename T>
inline SystemTag SysTag()
{
	return std::type_index(typeid(T));
}

template<typename T>
inline ResourceId ComponentRes()
{
	return { ResourceKind::Component, std::type_index(typeid(T)) };
}

template<typename T>
inline ResourceId EventRes()
{
	return { ResourceKind::EventQueue, std::type_index(typeid(T)) };
}

template<typename T>
inline ResourceId ExternalRes()
{
	return { ResourceKind::External, std::type_index(typeid(T)) };
}

struct DirtyTrackerResourceTag {};
struct CommandBufferResourceTag {};

inline ResourceId DirtyTrackerRes()
{
	return { ResourceKind::DirtyTracker, std::type_index(typeid(DirtyTrackerResourceTag)) };
}

inline ResourceId CommandBufferRes()
{
	return { ResourceKind::CommandBuffer, std::type_index(typeid(CommandBufferResourceTag)) };
}

inline AccessSpec ReadSnapshot(ResourceId res)
{
	return { res, AccessMode::Read, Visibility::Snapshot, StructuralEffect::None };
}

inline AccessSpec ReadImmediate(ResourceId res)
{
	return { res, AccessMode::Read, Visibility::Immediate, StructuralEffect::None };
}

inline AccessSpec WriteImmediate(
	ResourceId res,
	StructuralEffect se = StructuralEffect::None)
{
	return { res, AccessMode::Write, Visibility::Immediate, se };
}

inline AccessSpec WriteDeferred(
	ResourceId res,
	StructuralEffect se = StructuralEffect::None)
{
	return { res, AccessMode::Write, Visibility::Deferred, se };
}

inline AccessSpec AddRemoveImmediate(ResourceId res)
{
	return { res, AccessMode::Write, Visibility::Immediate, StructuralEffect::AddRemoveComponent };
}

inline AccessSpec CreateDestroyImmediate(ResourceId res)
{
	return { res, AccessMode::Write, Visibility::Immediate, StructuralEffect::CreateDestroyEntity };
}

inline AccessSpec AddRemoveDeferred(ResourceId res)
{
	return { res, AccessMode::Write, Visibility::Deferred, StructuralEffect::AddRemoveComponent };
}

inline AccessSpec CreateDestroyDeferred(ResourceId res)
{
	return { res, AccessMode::Write, Visibility::Deferred, StructuralEffect::CreateDestroyEntity };
}

inline AccessSpec EmitDeferred(ResourceId res)
{
	return { res, AccessMode::Emit, Visibility::Deferred, StructuralEffect::None };
}

inline AccessSpec ConsumeCommit(ResourceId res)
{
	return { res, AccessMode::Consume, Visibility::Commit, StructuralEffect::None };
}

inline AccessSpec ConsumeDeferred(ResourceId res)
{
	return { res, AccessMode::Consume, Visibility::Deferred, StructuralEffect::None };
}