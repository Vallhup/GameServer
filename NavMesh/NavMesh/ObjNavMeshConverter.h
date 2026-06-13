#pragma once

#include <cstddef>
#include <filesystem>

struct NavMeshBuildSettings
{
    float agentRadius = 0.35f;
    float agentHeight = 1.8f;
    float agentMaxClimb = 0.3f;
    float agentMaxSlope = 40.5f;

    float cellSize = 0.1f;
    float cellHeight = 0.1f;
    int tileSize = 16;

    float minRegionArea = 1.0f;
    float mergeRegionArea = 1.0f;
    float maxEdgeLength = 12.0f;
    float maxSimplificationError = 1.3f;
    int maxVerticesPerPolygon = 6;

    bool flipX = true;
    bool buildHeightMesh = true;
};

struct NavMeshBuildResult
{
    std::size_t inputVertexCount = 0;
    std::size_t inputTriangleCount = 0;
    int tileCountX = 0;
    int tileCountZ = 0;
    int writtenTileCount = 0;
};

class ObjNavMeshConverter
{
public:
    static NavMeshBuildResult Convert(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const NavMeshBuildSettings& settings);
};
