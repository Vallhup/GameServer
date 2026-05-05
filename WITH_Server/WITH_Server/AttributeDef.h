#pragma once

#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "GameplayContentIds.h"

#include <filesystem>
#include <span>
#include <string>

enum class AttributeValueKind : uint8_t
{
	Scalar,
	Resource
};

struct AttributeDef
{
	AttributeId id{ InvalidAttributeId };
	std::string key;
	std::string name;
	AttributeValueKind valueKind{ AttributeValueKind::Scalar };
	float defaultValue{ 0.0f };
	float minValue{ 0.0f };
	float maxValue{ 0.0f };
	bool clampToRange{ false };
};

bool ValidateAttributeDefs(
	std::span<const AttributeDef> defs,
	std::string& outError);

struct AttributeDefTraits
{
	static AttributeId GetId(const AttributeDef& def) noexcept
	{
		return def.id;
	}
};

using AttributeDefRegistry = DefRegistry<
	AttributeDef,
	AttributeId,
	AttributeDefTraits>;

DefLoadResult LoadAttributeDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AttributeDefRegistry& outRegistry);
