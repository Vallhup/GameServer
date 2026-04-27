#include "pch.h"
#include "DefJsonFileLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace
{
	bool ReadFileText(
		const std::filesystem::path& path,
		std::string& outText,
		std::string& outError)
	{
		std::ifstream input(path);
		if (!input.is_open())
		{
			outError = "Failed to open json file: " + path.string();
			return false;
		}

		std::stringstream buffer;
		buffer << input.rdbuf();
		outText = buffer.str();
		return true;
	}
}

DefLoadResult LoadDefJsonDocumentsFromDirectory(
	const std::filesystem::path& directory,
	std::vector<DefJsonDocument>& outDocuments,
	const char* defName)
{
	DefLoadResult result{};
	outDocuments.clear();

	std::error_code ec;
	if (!std::filesystem::exists(directory, ec) ||
		!std::filesystem::is_directory(directory, ec))
	{
		result.error = std::string(defName) +
			" json directory was not found: " + directory.string();
		return result;
	}

	std::vector<std::filesystem::path> files;
	for (const std::filesystem::directory_entry& entry :
		std::filesystem::directory_iterator(directory, ec))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".json")
			files.push_back(entry.path());
	}

	std::sort(files.begin(), files.end());
	if (files.empty())
	{
		result.error = std::string(defName) +
			" json directory has no json files: " + directory.string();
		return result;
	}

	outDocuments.reserve(files.size());
	for (const std::filesystem::path& file : files)
	{
		std::string text;
		if (!ReadFileText(file, text, result.error))
		{
			result.error = file.string() + ": " + result.error;
			return result;
		}

		DefJsonDocument document{};
		document.path = file;
		try
		{
			document.root = nlohmann::json::parse(text);
		}
		catch (const std::exception& ex)
		{
			result.error = file.string() +
				": Failed to parse json: " + std::string(ex.what());
			return result;
		}

		outDocuments.push_back(std::move(document));
	}

	result.succeeded = true;
	result.loadedCount = outDocuments.size();
	return result;
}
