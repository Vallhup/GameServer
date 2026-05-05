#pragma once

#include "DefJsonFileLoader.h"
#include "DefLoadResult.h"

#include <exception>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

template<typename TDto>
struct DefParsedDto
{
	std::filesystem::path sourcePath;
	TDto dto;
};

template<typename TDto>
using DefParsedDtoList = std::vector<DefParsedDto<TDto>>;

template<typename TDto, typename TAppendDocumentDtos>
bool ParseDefDtosFromJsonDocuments(
	std::span<const DefJsonDocument> documents,
	TAppendDocumentDtos appendDocumentDtos,
	DefParsedDtoList<TDto>& outDtos,
	std::string& outError)
{
	outDtos.clear();

	for (const DefJsonDocument& document : documents)
	{
		std::vector<TDto> documentDtos;
		try
		{
			if (!appendDocumentDtos(document.root, documentDtos, outError))
			{
				outError = document.path.string() + ": " + outError;
				return false;
			}
		}
		catch (const std::exception& ex)
		{
			outError = document.path.string() +
				": Invalid def json field: " + std::string(ex.what());
			return false;
		}

		outDtos.reserve(outDtos.size() + documentDtos.size());
		for (TDto& dto : documentDtos)
		{
			outDtos.push_back(DefParsedDto<TDto>{
				.sourcePath = document.path,
				.dto = std::move(dto)
			});
		}
	}

	return true;
}

template<typename TDto, typename TDef, typename TCompileDto>
bool CompileRuntimeDefs(
	std::span<const DefParsedDto<TDto>> dtos,
	TCompileDto compileDto,
	std::vector<TDef>& outDefs,
	std::string& outError)
{
	outDefs.clear();
	outDefs.reserve(dtos.size());

	for (const DefParsedDto<TDto>& parsedDto : dtos)
	{
		TDef def{};
		if (!compileDto(parsedDto.dto, def, outError))
		{
			outError = parsedDto.sourcePath.string() + ": " + outError;
			return false;
		}

		outDefs.push_back(std::move(def));
	}

	return true;
}

template<
	typename TDto,
	typename TDef,
	typename TRegistry,
	typename TAppendDocumentDtos,
	typename TCompileDto,
	typename TValidateDefs>
DefLoadResult LoadCompiledDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	const char* defName,
	TAppendDocumentDtos appendDocumentDtos,
	TCompileDto compileDto,
	TValidateDefs validateDefs,
	TRegistry& outRegistry)
{
	std::vector<DefJsonDocument> documents;
	DefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, defName);
	if (!result.succeeded)
		return result;

	DefParsedDtoList<TDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			appendDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<TDef> defs;
	if (!CompileRuntimeDefs(
			std::span<const DefParsedDto<TDto>>(dtos.data(), dtos.size()),
			compileDto,
			defs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	if (!validateDefs(
			std::span<const TDef>(defs.data(), defs.size()),
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	TRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}

	outRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = outRegistry.Size();
	result.error.clear();
	return result;
}
