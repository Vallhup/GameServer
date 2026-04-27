#pragma once

#include "json.hpp"

#include <string>

inline bool ReadRequiredString(
	const nlohmann::json& node,
	const char* field,
	std::string& outValue,
	std::string& outError)
{
	if (!node.contains(field) || !node.at(field).is_string())
	{
		outError = std::string("Missing or invalid string field: ") + field;
		return false;
	}

	outValue = node.at(field).get<std::string>();
	return true;
}

template<typename TValue>
bool ReadRequiredNumber(
	const nlohmann::json& node,
	const char* field,
	TValue& outValue,
	std::string& outError)
{
	if (!node.contains(field) || !node.at(field).is_number())
	{
		outError = std::string("Missing or invalid numeric field: ") + field;
		return false;
	}

	outValue = node.at(field).get<TValue>();
	return true;
}

template<typename TValue>
bool ReadOptionalNumber(
	const nlohmann::json& node,
	const char* field,
	TValue& outValue,
	std::string& outError)
{
	if (!node.contains(field))
		return true;

	if (!node.at(field).is_number())
	{
		outError = std::string("Invalid numeric field: ") + field;
		return false;
	}

	outValue = node.at(field).get<TValue>();
	return true;
}

inline bool ReadRequiredBool(
	const nlohmann::json& node,
	const char* field,
	bool& outValue,
	std::string& outError)
{
	if (!node.contains(field) || !node.at(field).is_boolean())
	{
		outError = std::string("Missing or invalid bool field: ") + field;
		return false;
	}

	outValue = node.at(field).get<bool>();
	return true;
}
