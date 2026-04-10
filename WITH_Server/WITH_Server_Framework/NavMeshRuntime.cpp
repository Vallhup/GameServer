#include "pch.h"
#include "NavMeshRuntime.h"

#include "Library/Detour/DetourNavMesh.h"
#include "Library/Detour/DetourNavMeshQuery.h"

#include <fstream>
#include <vector>

// ── 타일 파일 포맷 (Recast/Detour RecastDemo 샘플과 동일) ──────────────────
// 오프라인 빌드 도구도 이 포맷으로 직렬화해야 한다.

static constexpr int kNavMeshSetMagic   = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
static constexpr int kNavMeshSetVersion = 1;

struct NavMeshSetHeader
{
    int             magic;
    int             version;
    int             numTiles;
    dtNavMeshParams params;
};

struct NavMeshTileHeader
{
    dtTileRef tileRef;
    int       dataSize;
};

// ── NavMeshRuntime ────────────────────────────────────────────────────────────

NavMeshRuntime::~NavMeshRuntime()
{
    Unload();
}

void NavMeshRuntime::Unload()
{
    if (_query)
    {
        dtFreeNavMeshQuery(_query);
        _query = nullptr;
    }

    if (_navMesh)
    {
        dtFreeNavMesh(_navMesh);
        _navMesh = nullptr;
    }
}

bool NavMeshRuntime::LoadFromFile(const std::string& binFilePath)
{
    std::ifstream ifs(binFilePath, std::ios::binary);
    if (!ifs.is_open())
        return false;

    NavMeshSetHeader header{};
    ifs.read(reinterpret_cast<char*>(&header), sizeof(NavMeshSetHeader));

    if (ifs.fail())
        return false;

    if (header.magic != kNavMeshSetMagic || header.version != kNavMeshSetVersion)
        return false;

    dtNavMesh* navMesh = dtAllocNavMesh();
    if (navMesh == nullptr)
        return false;

    dtNavMeshQuery* query = nullptr;

    const dtStatus initNavMeshStatus = navMesh->init(&header.params);
    if (dtStatusFailed(initNavMeshStatus))
    {
        dtFreeNavMesh(navMesh);
        return false;
    }

    for (int i = 0; i < header.numTiles; ++i)
    {
        NavMeshTileHeader tileHeader{};
        ifs.read(reinterpret_cast<char*>(&tileHeader), sizeof(NavMeshTileHeader));

        if (ifs.fail())
        {
            dtFreeNavMesh(navMesh);
            return false;
        }

        // 현재 저장 포맷에서는 numTiles 개수만큼 유효한 타일 헤더가 와야 한다.
        if (tileHeader.tileRef == 0 || tileHeader.dataSize <= 0)
        {
            dtFreeNavMesh(navMesh);
            return false;
        }

        unsigned char* data =
            static_cast<unsigned char*>(dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM));
        if (data == nullptr)
        {
            dtFreeNavMesh(navMesh);
            return false;
        }

        ifs.read(reinterpret_cast<char*>(data), tileHeader.dataSize);
        if (ifs.fail())
        {
            dtFree(data);
            dtFreeNavMesh(navMesh);
            return false;
        }

        dtTileRef addedTileRef = 0;
        const dtStatus addTileStatus = navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, &addedTileRef);
        if (dtStatusFailed(addTileStatus))
        {
            dtFree(data);
            dtFreeNavMesh(navMesh);
            return false;
        }

        if (addedTileRef == 0)
        {
            dtFreeNavMesh(navMesh);
            return false;
        }
    }

    query = dtAllocNavMeshQuery();
    if (query == nullptr)
    {
        dtFreeNavMesh(navMesh);
        return false;
    }

    const dtStatus initQueryStatus = query->init(navMesh, 2048);
    if (dtStatusFailed(initQueryStatus))
    {
        dtFreeNavMeshQuery(query);
        dtFreeNavMesh(navMesh);
        return false;
    }

    Unload();

    _navMesh = navMesh;
    _query = query;
    return true;
}