#pragma once
#include <iostream>
#include <vector>
#include <Windows.h>
#include <DirectXMath.h>
#include "../Library/Include/FBX/fbxsdk.h"

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

class FBXLoader
{
public:
    FBXLoader();
    ~FBXLoader();

    bool Load(const std::wstring& fbxPath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);

private:
    void InitializeSdk();
    void DestroySdk();
    void ParseMesh(FbxMesh* mesh, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);
    void ProcessSafeVertex(FbxMesh* mesh, int polyIndex, int vertexIndex,
        bool hasUV, FbxStringList& uvSetNames,
        FbxVector4* ctrlPoints, int controlPointCount,
        std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);
    void ProcessNode(FbxNode* node, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices);

private:
    FbxManager* _manager = nullptr;
    FbxScene* _scene = nullptr;
};
