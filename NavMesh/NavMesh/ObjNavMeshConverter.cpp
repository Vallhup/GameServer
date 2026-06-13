#include "ObjNavMeshConverter.h"

#include "Library/Detour/DetourNavMesh.h"
#include "Library/Detour/DetourNavMeshBuilder.h"
#include "Library/Recast/Recast.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    constexpr int kNavMeshSetMagic = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
    constexpr int kNavMeshSetVersion = 1;
    constexpr int kReferenceBitBudget = 22;

    struct NavMeshSetHeader
    {
        int magic;
        int version;
        int numTiles;
        dtNavMeshParams params;
    };

    struct NavMeshTileHeader
    {
        dtTileRef tileRef;
        int dataSize;
    };

    static_assert(sizeof(dtTileRef) == 4, "The server MSET format expects 32-bit Detour tile references.");
    static_assert(sizeof(NavMeshSetHeader) == 40);
    static_assert(sizeof(NavMeshTileHeader) == 8);

    struct InputMesh
    {
        std::vector<float> vertices;
        std::vector<int> triangles;
        std::array<float, 3> boundsMin{};
        std::array<float, 3> boundsMax{};
    };

    struct RecastDeleter
    {
        void operator()(rcHeightfield* value) const { rcFreeHeightField(value); }
        void operator()(rcCompactHeightfield* value) const { rcFreeCompactHeightfield(value); }
        void operator()(rcContourSet* value) const { rcFreeContourSet(value); }
        void operator()(rcPolyMesh* value) const { rcFreePolyMesh(value); }
        void operator()(rcPolyMeshDetail* value) const { rcFreePolyMeshDetail(value); }
    };

    struct DetourNavMeshDeleter
    {
        void operator()(dtNavMesh* value) const { dtFreeNavMesh(value); }
    };

    using HeightfieldPtr = std::unique_ptr<rcHeightfield, RecastDeleter>;
    using CompactHeightfieldPtr = std::unique_ptr<rcCompactHeightfield, RecastDeleter>;
    using ContourSetPtr = std::unique_ptr<rcContourSet, RecastDeleter>;
    using PolyMeshPtr = std::unique_ptr<rcPolyMesh, RecastDeleter>;
    using PolyMeshDetailPtr = std::unique_ptr<rcPolyMeshDetail, RecastDeleter>;
    using NavMeshPtr = std::unique_ptr<dtNavMesh, DetourNavMeshDeleter>;

    int ResolveObjIndex(std::string_view token, std::size_t vertexCount, std::size_t lineNumber)
    {
        const std::size_t slash = token.find('/');
        const std::string indexText(token.substr(0, slash));
        if (indexText.empty())
            throw std::runtime_error("OBJ face has an empty vertex index at line " + std::to_string(lineNumber));

        std::size_t parsed = 0;
        const int objIndex = std::stoi(indexText, &parsed);
        if (parsed != indexText.size() || objIndex == 0)
            throw std::runtime_error("OBJ face has an invalid vertex index at line " + std::to_string(lineNumber));

        const long long resolved = objIndex > 0
            ? static_cast<long long>(objIndex) - 1
            : static_cast<long long>(vertexCount) + objIndex;

        if (resolved < 0 || resolved >= static_cast<long long>(vertexCount))
            throw std::runtime_error("OBJ face vertex index is out of range at line " + std::to_string(lineNumber));
        return static_cast<int>(resolved);
    }

    InputMesh LoadObj(const std::filesystem::path& path, bool flipX)
    {
        std::ifstream input(path);
        if (!input.is_open())
            throw std::runtime_error("Cannot open OBJ: " + path.string());

        InputMesh mesh;
        mesh.boundsMin.fill(std::numeric_limits<float>::max());
        mesh.boundsMax.fill(std::numeric_limits<float>::lowest());

        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line))
        {
            ++lineNumber;
            if (line.size() < 2)
                continue;

            if (line[0] == 'v' && std::isspace(static_cast<unsigned char>(line[1])))
            {
                std::istringstream stream(line.substr(2));
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                if (!(stream >> x >> y >> z))
                    throw std::runtime_error("Invalid OBJ vertex at line " + std::to_string(lineNumber));

                if (flipX)
                    x = -x;

                mesh.vertices.insert(mesh.vertices.end(), { x, y, z });
                mesh.boundsMin[0] = std::min(mesh.boundsMin[0], x);
                mesh.boundsMin[1] = std::min(mesh.boundsMin[1], y);
                mesh.boundsMin[2] = std::min(mesh.boundsMin[2], z);
                mesh.boundsMax[0] = std::max(mesh.boundsMax[0], x);
                mesh.boundsMax[1] = std::max(mesh.boundsMax[1], y);
                mesh.boundsMax[2] = std::max(mesh.boundsMax[2], z);
            }
            else if (line[0] == 'f' && std::isspace(static_cast<unsigned char>(line[1])))
            {
                std::istringstream stream(line.substr(2));
                std::vector<int> face;
                std::string token;
                while (stream >> token)
                    face.push_back(ResolveObjIndex(token, mesh.vertices.size() / 3, lineNumber));

                if (face.size() < 3)
                    throw std::runtime_error("OBJ face has fewer than three vertices at line " + std::to_string(lineNumber));

                for (std::size_t i = 1; i + 1 < face.size(); ++i)
                {
                    mesh.triangles.push_back(face[0]);
                    if (flipX)
                    {
                        // Reflecting X changes handedness. Reverse each triangle to preserve its normal.
                        mesh.triangles.push_back(face[i + 1]);
                        mesh.triangles.push_back(face[i]);
                    }
                    else
                    {
                        mesh.triangles.push_back(face[i]);
                        mesh.triangles.push_back(face[i + 1]);
                    }
                }
            }
        }

        if (mesh.vertices.empty())
            throw std::runtime_error("OBJ contains no vertices: " + path.string());
        if (mesh.triangles.empty())
            throw std::runtime_error("OBJ contains no faces: " + path.string());
        return mesh;
    }

    void ValidateSettings(const NavMeshBuildSettings& settings)
    {
        if (!(settings.agentRadius > 0.0f) ||
            !(settings.agentHeight > 0.0f) ||
            !(settings.agentMaxClimb >= 0.0f) ||
            !(settings.agentMaxSlope >= 0.0f && settings.agentMaxSlope < 90.0f) ||
            !(settings.cellSize > 0.0f) ||
            !(settings.cellHeight > 0.0f) ||
            settings.tileSize <= 0 ||
            settings.maxVerticesPerPolygon < 3 ||
            settings.maxVerticesPerPolygon > DT_VERTS_PER_POLYGON ||
            settings.minRegionArea < 0.0f ||
            settings.mergeRegionArea < 0.0f)
        {
            throw std::runtime_error("Invalid NavMesh build settings.");
        }
    }

    int NextPowerOfTwo(int value)
    {
        if (value <= 1)
            return 1;
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return value + 1;
    }

    int IntegerLog2(int value)
    {
        int result = 0;
        while (value > 1)
        {
            value >>= 1;
            ++result;
        }
        return result;
    }

    int ClampTileIndex(int value, int count)
    {
        return std::max(0, std::min(value, count - 1));
    }

    std::vector<std::vector<int>> BuildTriangleBuckets(
        const InputMesh& mesh,
        int tileCountX,
        int tileCountZ,
        float tileWorldSize,
        float borderWorldSize)
    {
        std::vector<std::vector<int>> buckets(
            static_cast<std::size_t>(tileCountX) * static_cast<std::size_t>(tileCountZ));

        const int triangleCount = static_cast<int>(mesh.triangles.size() / 3);
        for (int triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
        {
            float minX = std::numeric_limits<float>::max();
            float minZ = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float maxZ = std::numeric_limits<float>::lowest();

            for (int corner = 0; corner < 3; ++corner)
            {
                const int vertexIndex = mesh.triangles[triangleIndex * 3 + corner];
                const float x = mesh.vertices[vertexIndex * 3];
                const float z = mesh.vertices[vertexIndex * 3 + 2];
                minX = std::min(minX, x);
                minZ = std::min(minZ, z);
                maxX = std::max(maxX, x);
                maxZ = std::max(maxZ, z);
            }

            const int firstX = ClampTileIndex(
                static_cast<int>(std::floor((minX - borderWorldSize - mesh.boundsMin[0]) / tileWorldSize)) - 1,
                tileCountX);
            const int lastX = ClampTileIndex(
                static_cast<int>(std::floor((maxX + borderWorldSize - mesh.boundsMin[0]) / tileWorldSize)) + 1,
                tileCountX);
            const int firstZ = ClampTileIndex(
                static_cast<int>(std::floor((minZ - borderWorldSize - mesh.boundsMin[2]) / tileWorldSize)) - 1,
                tileCountZ);
            const int lastZ = ClampTileIndex(
                static_cast<int>(std::floor((maxZ + borderWorldSize - mesh.boundsMin[2]) / tileWorldSize)) + 1,
                tileCountZ);

            for (int z = firstZ; z <= lastZ; ++z)
            {
                const float tileMinZ = mesh.boundsMin[2] + z * tileWorldSize - borderWorldSize;
                const float tileMaxZ = tileMinZ + tileWorldSize + 2.0f * borderWorldSize;
                if (maxZ < tileMinZ || minZ > tileMaxZ)
                    continue;

                for (int x = firstX; x <= lastX; ++x)
                {
                    const float tileMinX = mesh.boundsMin[0] + x * tileWorldSize - borderWorldSize;
                    const float tileMaxX = tileMinX + tileWorldSize + 2.0f * borderWorldSize;
                    if (maxX < tileMinX || minX > tileMaxX)
                        continue;

                    buckets[static_cast<std::size_t>(z) * tileCountX + x].push_back(triangleIndex);
                }
            }
        }
        return buckets;
    }

    rcConfig MakeTileConfig(
        const InputMesh& mesh,
        const NavMeshBuildSettings& settings,
        int tileX,
        int tileZ)
    {
        rcConfig config{};
        config.cs = settings.cellSize;
        config.ch = settings.cellHeight;
        config.walkableSlopeAngle = settings.agentMaxSlope;
        config.walkableHeight = static_cast<int>(std::ceil(settings.agentHeight / config.ch));
        config.walkableClimb = static_cast<int>(std::floor(settings.agentMaxClimb / config.ch + 0.0001f));
        config.walkableRadius = static_cast<int>(std::ceil(settings.agentRadius / config.cs));
        config.maxEdgeLen = static_cast<int>(settings.maxEdgeLength / config.cs);
        config.maxSimplificationError = settings.maxSimplificationError;
        config.minRegionArea = static_cast<int>(std::ceil(
            settings.minRegionArea / (config.cs * config.cs)));
        config.mergeRegionArea = static_cast<int>(std::ceil(
            settings.mergeRegionArea / (config.cs * config.cs)));
        config.maxVertsPerPoly = settings.maxVerticesPerPolygon;
        config.tileSize = settings.tileSize;
        config.borderSize = config.walkableRadius + 3;
        config.width = config.tileSize + config.borderSize * 2;
        config.height = config.tileSize + config.borderSize * 2;

        const float tileWorldSize = config.tileSize * config.cs;
        const float borderWorldSize = config.borderSize * config.cs;
        config.bmin[0] = mesh.boundsMin[0] + tileX * tileWorldSize - borderWorldSize;
        config.bmin[1] = mesh.boundsMin[1];
        config.bmin[2] = mesh.boundsMin[2] + tileZ * tileWorldSize - borderWorldSize;
        config.bmax[0] = config.bmin[0] + tileWorldSize + borderWorldSize * 2.0f;
        config.bmax[1] = mesh.boundsMax[1] + settings.agentHeight;
        config.bmax[2] = config.bmin[2] + tileWorldSize + borderWorldSize * 2.0f;
        return config;
    }

    std::vector<unsigned char> BuildTile(
        rcContext& context,
        const InputMesh& mesh,
        const std::vector<int>& triangleIndices,
        const NavMeshBuildSettings& settings,
        int tileX,
        int tileZ)
    {
        if (triangleIndices.empty())
            return {};

        const rcConfig config = MakeTileConfig(mesh, settings, tileX, tileZ);

        std::vector<int> triangles;
        triangles.reserve(triangleIndices.size() * 3);
        for (const int triangleIndex : triangleIndices)
        {
            const int offset = triangleIndex * 3;
            triangles.insert(
                triangles.end(),
                mesh.triangles.begin() + offset,
                mesh.triangles.begin() + offset + 3);
        }

        std::vector<unsigned char> triangleAreas(triangleIndices.size(), RC_NULL_AREA);
        rcMarkWalkableTriangles(
            &context,
            config.walkableSlopeAngle,
            mesh.vertices.data(),
            static_cast<int>(mesh.vertices.size() / 3),
            triangles.data(),
            static_cast<int>(triangleIndices.size()),
            triangleAreas.data());

        if (std::none_of(
                triangleAreas.begin(),
                triangleAreas.end(),
                [](unsigned char area) { return area != RC_NULL_AREA; }))
        {
            return {};
        }

        HeightfieldPtr heightfield(rcAllocHeightfield());
        if (!heightfield ||
            !rcCreateHeightfield(
                &context,
                *heightfield,
                config.width,
                config.height,
                config.bmin,
                config.bmax,
                config.cs,
                config.ch))
        {
            throw std::runtime_error("Could not create Recast heightfield.");
        }

        if (!rcRasterizeTriangles(
                &context,
                mesh.vertices.data(),
                static_cast<int>(mesh.vertices.size() / 3),
                triangles.data(),
                triangleAreas.data(),
                static_cast<int>(triangleIndices.size()),
                *heightfield,
                config.walkableClimb))
        {
            throw std::runtime_error("Could not rasterize OBJ triangles.");
        }

        rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *heightfield);
        rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *heightfield);
        rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *heightfield);

        CompactHeightfieldPtr compactHeightfield(rcAllocCompactHeightfield());
        if (!compactHeightfield ||
            !rcBuildCompactHeightfield(
                &context,
                config.walkableHeight,
                config.walkableClimb,
                *heightfield,
                *compactHeightfield))
        {
            throw std::runtime_error("Could not build compact heightfield.");
        }
        heightfield.reset();

        if (config.walkableRadius > 0 &&
            !rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield))
        {
            throw std::runtime_error("Could not erode walkable area.");
        }

        if (!rcBuildDistanceField(&context, *compactHeightfield) ||
            !rcBuildRegions(
                &context,
                *compactHeightfield,
                config.borderSize,
                config.minRegionArea,
                config.mergeRegionArea))
        {
            throw std::runtime_error("Could not build Recast regions.");
        }

        ContourSetPtr contourSet(rcAllocContourSet());
        if (!contourSet ||
            !rcBuildContours(
                &context,
                *compactHeightfield,
                config.maxSimplificationError,
                config.maxEdgeLen,
                *contourSet))
        {
            throw std::runtime_error("Could not build Recast contours.");
        }

        PolyMeshPtr polyMesh(rcAllocPolyMesh());
        if (!polyMesh ||
            !rcBuildPolyMesh(
                &context,
                *contourSet,
                config.maxVertsPerPoly,
                *polyMesh))
        {
            throw std::runtime_error("Could not build Recast polygon mesh.");
        }

        if (polyMesh->npolys == 0)
            return {};

        PolyMeshDetailPtr detailMesh;
        if (settings.buildHeightMesh)
        {
            detailMesh.reset(rcAllocPolyMeshDetail());
            const float detailSampleDistance = std::max(0.9f, config.cs * 6.0f);
            const float detailSampleMaxError = config.ch;
            if (!detailMesh ||
                !rcBuildPolyMeshDetail(
                    &context,
                    *polyMesh,
                    *compactHeightfield,
                    detailSampleDistance,
                    detailSampleMaxError,
                    *detailMesh))
            {
                throw std::runtime_error("Could not build Recast detail mesh.");
            }
        }

        for (int polygonIndex = 0; polygonIndex < polyMesh->npolys; ++polygonIndex)
        {
            if (polyMesh->areas[polygonIndex] == RC_WALKABLE_AREA)
                polyMesh->areas[polygonIndex] = 0;
            polyMesh->flags[polygonIndex] = 1;
        }

        dtNavMeshCreateParams params{};
        params.verts = polyMesh->verts;
        params.vertCount = polyMesh->nverts;
        params.polys = polyMesh->polys;
        params.polyAreas = polyMesh->areas;
        params.polyFlags = polyMesh->flags;
        params.polyCount = polyMesh->npolys;
        params.nvp = polyMesh->nvp;
        if (detailMesh)
        {
            params.detailMeshes = detailMesh->meshes;
            params.detailVerts = detailMesh->verts;
            params.detailVertsCount = detailMesh->nverts;
            params.detailTris = detailMesh->tris;
            params.detailTriCount = detailMesh->ntris;
        }
        params.walkableHeight = settings.agentHeight;
        params.walkableRadius = settings.agentRadius;
        params.walkableClimb = settings.agentMaxClimb;
        params.tileX = tileX;
        params.tileY = tileZ;
        params.tileLayer = 0;
        params.cs = config.cs;
        params.ch = config.ch;
        params.buildBvTree = true;
        std::memcpy(params.bmin, polyMesh->bmin, sizeof(params.bmin));
        std::memcpy(params.bmax, polyMesh->bmax, sizeof(params.bmax));

        unsigned char* rawData = nullptr;
        int rawDataSize = 0;
        if (!dtCreateNavMeshData(&params, &rawData, &rawDataSize))
            throw std::runtime_error("Could not create Detour tile data.");

        std::vector<unsigned char> data(rawData, rawData + rawDataSize);
        dtFree(rawData);
        return data;
    }

    void SaveNavMeshSet(const std::filesystem::path& path, const dtNavMesh& navMesh)
    {
        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output.is_open())
            throw std::runtime_error("Cannot open output file: " + path.string());

        NavMeshSetHeader header{};
        header.magic = kNavMeshSetMagic;
        header.version = kNavMeshSetVersion;
        header.params = *navMesh.getParams();

        for (int i = 0; i < navMesh.getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = navMesh.getTile(i);
            if (tile != nullptr && tile->header != nullptr && tile->dataSize > 0)
                ++header.numTiles;
        }

        output.write(reinterpret_cast<const char*>(&header), sizeof(header));
        for (int i = 0; i < navMesh.getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = navMesh.getTile(i);
            if (tile == nullptr || tile->header == nullptr || tile->dataSize <= 0)
                continue;

            const NavMeshTileHeader tileHeader{
                navMesh.getTileRef(tile),
                tile->dataSize
            };
            output.write(reinterpret_cast<const char*>(&tileHeader), sizeof(tileHeader));
            output.write(reinterpret_cast<const char*>(tile->data), tile->dataSize);
        }

        if (!output)
            throw std::runtime_error("Failed while writing output file: " + path.string());
    }

    void ValidateNavMeshSet(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
            throw std::runtime_error("Cannot reopen output file: " + path.string());

        NavMeshSetHeader header{};
        input.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!input ||
            header.magic != kNavMeshSetMagic ||
            header.version != kNavMeshSetVersion ||
            header.numTiles <= 0)
        {
            throw std::runtime_error("Output file has an invalid MSET header.");
        }

        NavMeshPtr navMesh(dtAllocNavMesh());
        if (!navMesh || dtStatusFailed(navMesh->init(&header.params)))
            throw std::runtime_error("Output MSET parameters cannot initialize Detour.");

        for (int i = 0; i < header.numTiles; ++i)
        {
            NavMeshTileHeader tileHeader{};
            input.read(reinterpret_cast<char*>(&tileHeader), sizeof(tileHeader));
            if (!input || tileHeader.tileRef == 0 || tileHeader.dataSize <= 0)
                throw std::runtime_error("Output file has an invalid tile header.");

            unsigned char* data = static_cast<unsigned char*>(
                dtAlloc(static_cast<std::size_t>(tileHeader.dataSize), DT_ALLOC_PERM));
            if (data == nullptr)
                throw std::runtime_error("Could not allocate memory while validating output.");

            input.read(reinterpret_cast<char*>(data), tileHeader.dataSize);
            if (!input)
            {
                dtFree(data);
                throw std::runtime_error("Output file ends inside tile data.");
            }

            const dtStatus status = navMesh->addTile(
                data,
                tileHeader.dataSize,
                DT_TILE_FREE_DATA,
                tileHeader.tileRef,
                nullptr);
            if (dtStatusFailed(status))
            {
                dtFree(data);
                throw std::runtime_error("Detour rejected a tile from the saved output.");
            }
        }

        if (input.peek() != std::ifstream::traits_type::eof())
            throw std::runtime_error("Output file has unexpected trailing data.");
    }
}

NavMeshBuildResult ObjNavMeshConverter::Convert(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const NavMeshBuildSettings& settings)
{
    ValidateSettings(settings);
    const InputMesh mesh = LoadObj(inputPath, settings.flipX);

    const float tileWorldSize = settings.tileSize * settings.cellSize;
    const int tileCountX = std::max(
        1,
        static_cast<int>(std::ceil((mesh.boundsMax[0] - mesh.boundsMin[0]) / tileWorldSize)));
    const int tileCountZ = std::max(
        1,
        static_cast<int>(std::ceil((mesh.boundsMax[2] - mesh.boundsMin[2]) / tileWorldSize)));

    const long long gridTileCount =
        static_cast<long long>(tileCountX) * static_cast<long long>(tileCountZ);
    if (gridTileCount > (1 << kReferenceBitBudget))
        throw std::runtime_error("Tile grid is too large for 32-bit Detour references.");

    const int maxTiles = NextPowerOfTwo(static_cast<int>(gridTileCount));
    const int tileBits = IntegerLog2(maxTiles);
    const int polyBits = kReferenceBitBudget - tileBits;
    if (polyBits <= 0)
        throw std::runtime_error("Not enough Detour reference bits remain for polygons.");
    const int maxPolygonsPerTile = 1 << polyBits;

    dtNavMeshParams navMeshParams{};
    std::memcpy(navMeshParams.orig, mesh.boundsMin.data(), sizeof(navMeshParams.orig));
    navMeshParams.tileWidth = tileWorldSize;
    navMeshParams.tileHeight = tileWorldSize;
    navMeshParams.maxTiles = maxTiles;
    navMeshParams.maxPolys = maxPolygonsPerTile;

    NavMeshPtr navMesh(dtAllocNavMesh());
    if (!navMesh || dtStatusFailed(navMesh->init(&navMeshParams)))
        throw std::runtime_error("Could not initialize Detour NavMesh.");

    const int walkableRadius = static_cast<int>(std::ceil(settings.agentRadius / settings.cellSize));
    const int borderSize = walkableRadius + 3;
    const float borderWorldSize = borderSize * settings.cellSize;
    const std::vector<std::vector<int>> buckets = BuildTriangleBuckets(
        mesh,
        tileCountX,
        tileCountZ,
        tileWorldSize,
        borderWorldSize);

    rcContext context(false);
    int writtenTiles = 0;
    for (int z = 0; z < tileCountZ; ++z)
    {
        for (int x = 0; x < tileCountX; ++x)
        {
            const std::vector<unsigned char> tileData = BuildTile(
                context,
                mesh,
                buckets[static_cast<std::size_t>(z) * tileCountX + x],
                settings,
                x,
                z);
            if (tileData.empty())
                continue;

            unsigned char* ownedData = static_cast<unsigned char*>(
                dtAlloc(tileData.size(), DT_ALLOC_PERM));
            if (ownedData == nullptr)
                throw std::runtime_error("Could not allocate Detour tile data.");
            std::memcpy(ownedData, tileData.data(), tileData.size());

            const dtStatus status = navMesh->addTile(
                ownedData,
                static_cast<int>(tileData.size()),
                DT_TILE_FREE_DATA,
                0,
                nullptr);
            if (dtStatusFailed(status))
            {
                dtFree(ownedData);
                throw std::runtime_error(
                    "Could not add tile (" + std::to_string(x) + ", " + std::to_string(z) +
                    ") to Detour NavMesh. The tile may exceed maxPolys=" +
                    std::to_string(maxPolygonsPerTile) + ".");
            }

            ++writtenTiles;
        }

        if ((z + 1) % 10 == 0 || z + 1 == tileCountZ)
        {
            std::cout
                << "\rBuilding tiles: " << (z + 1) << '/' << tileCountZ
                << " rows, " << writtenTiles << " tiles" << std::flush;
        }
    }
    std::cout << '\n';

    if (writtenTiles == 0)
        throw std::runtime_error("Recast produced no walkable tiles.");

    SaveNavMeshSet(outputPath, *navMesh);
    ValidateNavMeshSet(outputPath);

    NavMeshBuildResult result;
    result.inputVertexCount = mesh.vertices.size() / 3;
    result.inputTriangleCount = mesh.triangles.size() / 3;
    result.tileCountX = tileCountX;
    result.tileCountZ = tileCountZ;
    result.writtenTileCount = writtenTiles;
    return result;
}
