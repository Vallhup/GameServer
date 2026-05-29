#include "pch.h"
#include "TitleDef.h"

#include "DefCompilePipeline.h"
#include "DefEnumString.h"
#include "DefJsonReader.h"
#include "DefRegistry.h"
#include "json.hpp"

using json = nlohmann::json;

namespace
{
    template<typename TEnum>
    bool ReadEnum(
        const json& node,
        const char* field,
        TEnum& outValue,
        const char* typeName,
        std::string& outError)
    {
        std::string text;
        if (!ReadRequiredString(node, field, text, outError))
            return false;

        if (!ParseDefString(text, outValue))
        {
            outError = std::string("Unknown ") + typeName + ": " + text;
            return false;
        }

        return true;
    }

    bool ParseTitleDocument(
        const json& root,
        TitleDef& outDef,
        std::string& outError)
    {
        if (!ReadRequiredNumber(root, "titleId", outDef.id, outError))
            return false;

        if (!ReadRequiredString(root, "displayName", outDef.displayName, outError))
            return false;

        if (!ReadEnum(
                root,
                "conditionType",
                outDef.conditionType,
                "title condition type",
                outError))
        {
            return false;
        }

        if (!ReadEnum(
                root,
                "characterId",
                outDef.characterId,
                "character id",
                outError))
        {
            return false;
        }

        if (!ReadRequiredNumber(root, "requiredCount", outDef.requiredCount, outError))
            return false;

        return true;
    }

    bool ValidateTitleDefs(
        std::span<const TitleDef> defs,
        std::string& outError)
    {
        for (const TitleDef& def : defs)
        {
            if (def.id == InvalidTitleId)
            {
                outError = "Title id must not be 0.";
                return false;
            }

            if (def.displayName.empty())
            {
                outError = "Title displayName must not be empty.";
                return false;
            }

            if (def.conditionType == TitleConditionType::None)
            {
                outError = "Title conditionType must not be None.";
                return false;
            }

            if (def.requiredCount <= 0)
            {
                outError = "Title requiredCount must be greater than 0.";
                return false;
            }
        }

        return true;
    }

    bool AppendTitleDocumentDtos(
        const json& root,
        std::vector<TitleDef>& outDtos,
        std::string& outError)
    {
        TitleDef def{};
        if (!ParseTitleDocument(root, def, outError))
            return false;

        outDtos.push_back(std::move(def));
        return true;
    }

    bool CompileTitleDef(
        const TitleDef& dto,
        TitleDef& outDef,
        std::string& outError)
    {
        (void)outError;
        outDef = dto;
        return true;
    }
}

TitleDefLoadResult LoadTitleDefsFromJsonDirectory(
    const std::filesystem::path& directory,
    TitleDefRegistry& outRegistry)
{
    return LoadCompiledDefsFromJsonDirectory<TitleDef, TitleDef>(
        directory,
        "Title",
        AppendTitleDocumentDtos,
        CompileTitleDef,
        ValidateTitleDefs,
        outRegistry);
}
