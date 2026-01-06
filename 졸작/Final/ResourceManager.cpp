#include "pch.h"
#include "ResourceManager.h"
#include "VertexIndexBuffer.h"
#include "Material.h"

shared_ptr<CachedMeshData> ResourceManager::GetCachedMesh(const wstring& path)
{
    auto it = meshCache.find(path);
    if (it != meshCache.end()) {
        OutputDebugStringA(("Mesh cache HIT: " + string(path.begin(), path.end()) + "\n").c_str());
        return it->second;
    }

    OutputDebugStringA(("Mesh cache MISS: " + string(path.begin(), path.end()) + "\n").c_str());
    return nullptr;
}

void ResourceManager::CacheMesh(const wstring& path,
    const shared_ptr<VertexIndexBuffer>& vib,
    const vector<UINT>& materialIndices,
    const vector<SubMeshInfo>& subMeshes,
    const vector<MaterialData>& originalData,
    bool hasanimation,
    const vector<AnimClipInfo>& animationclips,
    const SkeletonData& skeletondata,
    const BoundingBox& box)
{
    // 이미 캐시된 경우 무시
    if (meshCache.find(path) != meshCache.end()) {
        return;
    }

    auto cachedData = make_shared<CachedMeshData>();

    cachedData->vertexIndexBuffer = vib;
    cachedData->materialIndices = materialIndices;
    cachedData->subMeshes = subMeshes;
    cachedData->originalMaterialData = originalData;
    cachedData->hasAnimation = hasanimation;
    cachedData->animationClips = animationclips;
    cachedData->skeletonData = skeletondata;
    cachedData->boundingBox = box;

    meshCache[path] = cachedData;

    OutputDebugStringA(("Mesh cached: " + string(path.begin(), path.end()) + "\n").c_str());
}

void ResourceManager::ClearCache()
{
    meshCache.clear();
    OutputDebugStringA("Resource cache cleared!\n");
}

void ResourceManager::PrintCacheStatus()
{
    OutputDebugStringA("=== Resource Cache Status ===\n");
    OutputDebugStringA(("Cached meshes: " + to_string(meshCache.size()) + "\n").c_str());

    for (const auto& pair : meshCache) {
        string pathStr(pair.first.begin(), pair.first.end());
        OutputDebugStringA(("  - " + pathStr + "\n").c_str());
    }
    OutputDebugStringA("=============================\n");
}