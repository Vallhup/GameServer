#include "ObjNavMeshConverter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    void PrintUsage()
    {
        std::cout
            << "Usage:\n"
            << "  NavMesh.exe\n"
            << "  NavMesh.exe <input.obj> <output.bin> [options]\n\n"
            << "With no arguments, every Resource/*.obj file is converted to Output/<name>.bin.\n\n"
            << "Unity profile defaults:\n"
            << "  --agent-radius <value>       0.35\n"
            << "  --agent-height <value>       1.8\n"
            << "  --agent-climb <value>        0.3\n"
            << "  --agent-slope <degrees>      40.5\n"
            << "  --cell-size <value>          0.1\n"
            << "  --cell-height <value>        0.1\n"
            << "  --tile-size <voxels>         16\n"
            << "  --min-region-area <value>    1.0 square world units\n"
            << "  --merge-region-area <value>  1.0 square world units\n"
            << "  --no-flip-x                  Keep the OBJ X coordinate unchanged\n"
            << "  --no-height-mesh             Skip Recast detail mesh generation\n";
    }

    float ParseFloat(const char* value, std::string_view option)
    {
        char* end = nullptr;
        const float result = std::strtof(value, &end);
        if (end == value || *end != '\0')
            throw std::runtime_error("Invalid value for " + std::string(option) + ": " + value);
        return result;
    }

    int ParseInt(const char* value, std::string_view option)
    {
        char* end = nullptr;
        const long result = std::strtol(value, &end, 10);
        if (end == value || *end != '\0')
            throw std::runtime_error("Invalid value for " + std::string(option) + ": " + value);
        return static_cast<int>(result);
    }

std::filesystem::path FindProjectRoot(const char* executablePath)
{
    const std::array starts{
        std::filesystem::current_path(),
        std::filesystem::absolute(executablePath).parent_path()
    };

    for (const std::filesystem::path& start : starts)
    {
        for (std::filesystem::path current = start; !current.empty(); current = current.parent_path())
        {
            if (std::filesystem::is_directory(current / "Resource"))
                return current;
            if (current == current.root_path())
                break;
        }
    }

    throw std::runtime_error("Cannot find the project Resource directory.");
}

void PrintResult(
    const std::filesystem::path& outputPath,
    const NavMeshBuildResult& result)
{
    std::cout
        << "Created " << outputPath.string() << '\n'
        << "  vertices: " << result.inputVertexCount << '\n'
        << "  triangles: " << result.inputTriangleCount << '\n'
        << "  grid: " << result.tileCountX << " x " << result.tileCountZ << '\n'
        << "  written tiles: " << result.writtenTileCount << '\n';
}

void ConvertAllResources(
    const char* executablePath,
    const NavMeshBuildSettings& settings)
{
    const std::filesystem::path root = FindProjectRoot(executablePath);
    const std::filesystem::path resourceDirectory = root / "Resource";
    const std::filesystem::path outputDirectory = root / "Output";

    std::vector<std::filesystem::path> inputPaths;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(resourceDirectory))
    {
        if (!entry.is_regular_file())
            continue;

        std::string extension = entry.path().extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (extension == ".obj")
            inputPaths.push_back(entry.path());
    }

    std::sort(inputPaths.begin(), inputPaths.end());
    if (inputPaths.empty())
        throw std::runtime_error("No OBJ files found in " + resourceDirectory.string());

    ObjNavMeshConverter converter;
    for (const std::filesystem::path& inputPath : inputPaths)
    {
        const std::filesystem::path outputPath =
            outputDirectory / (inputPath.stem().string() + ".bin");
        std::cout << "Converting " << inputPath.filename().string() << "...\n";
        PrintResult(outputPath, converter.Convert(inputPath, outputPath, settings));
    }
}
}

int main(int argc, char* argv[])
{
    if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h"))
    {
        PrintUsage();
        return 0;
    }

    if (argc == 2)
    {
        PrintUsage();
        return 1;
    }

    try
    {
        NavMeshBuildSettings settings;

        if (argc == 1)
        {
            ConvertAllResources(argv[0], settings);
            return 0;
        }

        for (int i = 3; i < argc; ++i)
        {
            const std::string_view option = argv[i];
            const auto requireValue = [&]() -> const char*
            {
                if (++i >= argc)
                    throw std::runtime_error("Missing value for " + std::string(option));
                return argv[i];
            };

            if (option == "--agent-radius")
                settings.agentRadius = ParseFloat(requireValue(), option);
            else if (option == "--agent-height")
                settings.agentHeight = ParseFloat(requireValue(), option);
            else if (option == "--agent-climb")
                settings.agentMaxClimb = ParseFloat(requireValue(), option);
            else if (option == "--agent-slope")
                settings.agentMaxSlope = ParseFloat(requireValue(), option);
            else if (option == "--cell-size")
                settings.cellSize = ParseFloat(requireValue(), option);
            else if (option == "--cell-height")
                settings.cellHeight = ParseFloat(requireValue(), option);
            else if (option == "--tile-size")
                settings.tileSize = ParseInt(requireValue(), option);
            else if (option == "--min-region-area")
                settings.minRegionArea = ParseFloat(requireValue(), option);
            else if (option == "--merge-region-area")
                settings.mergeRegionArea = ParseFloat(requireValue(), option);
            else if (option == "--no-flip-x")
                settings.flipX = false;
            else if (option == "--no-height-mesh")
                settings.buildHeightMesh = false;
            else if (option == "--help" || option == "-h")
            {
                PrintUsage();
                return 0;
            }
            else
            {
                throw std::runtime_error("Unknown option: " + std::string(option));
            }
        }

        const std::filesystem::path inputPath = argv[1];
        const std::filesystem::path outputPath = argv[2];

        ObjNavMeshConverter converter;
        const NavMeshBuildResult result = converter.Convert(inputPath, outputPath, settings);
        PrintResult(outputPath, result);
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "NavMesh conversion failed: " << exception.what() << '\n';
        return 1;
    }
}
