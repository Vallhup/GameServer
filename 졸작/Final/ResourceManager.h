#pragma once
#include "Importer.h"

class VertexIndexBuffer;
class Material;

struct CachedMeshData
{
    shared_ptr<VertexIndexBuffer> vertexIndexBuffer;
    vector<UINT> materialIndices;
    vector<SubMeshInfo> subMeshes;
    vector<MaterialData> originalMaterialData;
    bool hasAnimation = false;

    vector<AnimClipInfo> animationClips;    
    SkeletonData skeletonData;              
};

class ResourceManager
{
public:
    static ResourceManager& Get();

    shared_ptr<CachedMeshData> GetCachedMesh(const wstring& path);
    void CacheMesh(const wstring& path,
        const shared_ptr<VertexIndexBuffer>& vib,
        const vector<UINT>& materialIndices,
        const vector<SubMeshInfo>& subMeshes,
        const vector<MaterialData>& originalData,
        bool hasanimation,
        const vector<AnimClipInfo>& animationclips,
        const SkeletonData& skeletondata);

    void ClearCache();
    void PrintCacheStatus();

private:
	unordered_map <wstring, shared_ptr<CachedMeshData>> meshCache;
};

