#pragma once

#include <span>
#include <cstdint>
#include <typeindex>
#include <string_view>

using SystemTag = std::type_index;

enum class ResourceKind : uint8_t
{
	Component,
	EventQueue,
	DirtyTraker,
	CommandBuffer,
	External
};

enum class AccessMode : uint8_t
{
	Read,
	Write,
	Emit,
	Consume
};

enum class Visibility : uint8_t
{
	Snapshot,
	Immediate,
	Deferred,
	Commit
};

enum class StructuralEffect : uint8_t
{
	None,
	AddRemoveComponent,
	CreateDestroyEntity
};

struct ResourceId
{
	ResourceKind kind;
	std::type_index type;

	bool operator==(const ResourceId& rhs) const
	{
		return kind == rhs.kind && type == rhs.type;
	}
};

struct AccessSpec
{
	ResourceId resource;
	AccessMode mode;
	Visibility visibility;
	StructuralEffect effect{ StructuralEffect::None };
};

struct SystemMeta
{
	SystemTag tag;
	std::string_view name;

	std::span<const AccessSpec> accesses;
	std::span<const SystemTag> runsBefore;
	std::span<const SystemTag> runsAfter;
};