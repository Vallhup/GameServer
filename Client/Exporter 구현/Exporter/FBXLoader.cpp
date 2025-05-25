#include "FBXLoader.h"

FBXLoader::FBXLoader()
{
    InitializeSdk();
}

FBXLoader::~FBXLoader()
{
    DestroySdk();
}

void FBXLoader::InitializeSdk()
{
    _manager = FbxManager::Create();

    FbxIOSettings* ioSettings = FbxIOSettings::Create(_manager, IOSROOT);
    _manager->SetIOSettings(ioSettings);

    _scene = FbxScene::Create(_manager, "Scene");
}

void FBXLoader::DestroySdk()
{
    if (_scene)
        _scene->Destroy();
    if (_manager)
        _manager->Destroy();
}

bool FBXLoader::Load(const std::wstring& fbxPath, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
    FbxImporter* importer = FbxImporter::Create(_manager, "");

    std::string pathA(fbxPath.begin(), fbxPath.end()); // wstring to ANSI
    if (!importer->Initialize(pathA.c_str(), -1, _manager->GetIOSettings()))
    {
        importer->Destroy();
        return false;
    }

    if (!importer->Import(_scene))
    {
        importer->Destroy();
        return false;
    }

    importer->Destroy();

    FbxNode* rootNode = _scene->GetRootNode();
    if (!rootNode) return false;

    for (int i = 0; i < rootNode->GetChildCount(); ++i)
    {
        FbxNode* child = rootNode->GetChild(i);
        FbxMesh* mesh = child->GetMesh();
        if (mesh)
        {
            ParseMesh(mesh, outVertices, outIndices);
            break; // 하나만 처리
        }
    }

    return true;
}

void FBXLoader::ParseMesh(FbxMesh* mesh, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
    FbxVector4* ctrlPoints = mesh->GetControlPoints();
    int polygonCount = mesh->GetPolygonCount();

    int indexCount = 0;

    FbxStringList uvSetNames;
    mesh->GetUVSetNames(uvSetNames);

    for (int i = 0; i < polygonCount; ++i)
    {
        int polygonSize = mesh->GetPolygonSize(i);
        if (polygonSize != 3) continue; // 삼각형만

        for (int j = 0; j < 3; ++j)
        {
            int cpIndex = mesh->GetPolygonVertex(i, j);
            FbxVector4 pos = ctrlPoints[cpIndex];

            Vertex v = {};
            v.position = XMFLOAT3((float)pos[0], (float)pos[1], (float)pos[2]);

            // 노멀
            FbxVector4 normal;
            mesh->GetPolygonVertexNormal(i, j, normal);
            normal.Normalize();
            v.normal = XMFLOAT3((float)normal[0], (float)normal[1], (float)normal[2]);

            // UV
            FbxVector2 uv(0, 0);
            if (uvSetNames.GetCount() > 0)
            {
                bool unmapped;
                mesh->GetPolygonVertexUV(i, j, uvSetNames[0], uv, unmapped);
                v.uv = XMFLOAT2((float)uv[0], (float)uv[1]);
            }

            outVertices.push_back(v);
            outIndices.push_back((UINT)outVertices.size() - 1);
        }
    }
}
