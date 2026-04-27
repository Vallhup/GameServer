#pragma once

#include "DefLoadResult.h"
#include "json.hpp"

#include <filesystem>
#include <vector>

struct DefJsonDocument
{
	std::filesystem::path path;
	nlohmann::json root;
};

DefLoadResult LoadDefJsonDocumentsFromDirectory(
	const std::filesystem::path& directory,
	std::vector<DefJsonDocument>& outDocuments,
	const char* defName);
