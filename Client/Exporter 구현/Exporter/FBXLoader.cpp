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
    std::string pathA(fbxPath.begin(), fbxPath.end());

    if (!importer->Initialize(pathA.c_str(), -1, _manager->GetIOSettings()))
    {
        std::cout << "FBX Importer Initialize failed: " << importer->GetStatus().GetErrorString() << std::endl;
        importer->Destroy();
        return false;
    }

    if (!importer->Import(_scene))
    {
        std::cout << "FBX Import failed: " << importer->GetStatus().GetErrorString() << std::endl;
        importer->Destroy();
        return false;
    }

    importer->Destroy();

    // 씬 전체 탐색 - 모든 메쉬 처리
    ProcessNode(_scene->GetRootNode(), outVertices, outIndices);

    return !outVertices.empty();
}

void FBXLoader::ParseMesh(FbxMesh* mesh, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
    if (!mesh) {
        std::cout << "Mesh is null" << std::endl;
        return;
    }

    std::cout << "Processing mesh: " << mesh->GetName() << std::endl;

    try {
        // 1. 강제 삼각화 (모든 FBX에 적용)
        FbxGeometryConverter converter(mesh->GetScene()->GetFbxManager());
        FbxMesh* workingMesh = mesh;

        if (!mesh->IsTriangleMesh())
        {
            std::cout << "Converting to triangles..." << std::endl;
            FbxNodeAttribute* triangulatedAttr = converter.Triangulate(mesh, true);
            if (triangulatedAttr && triangulatedAttr->GetAttributeType() == FbxNodeAttribute::eMesh) {
                workingMesh = (FbxMesh*)triangulatedAttr;
                std::cout << "Triangulation successful" << std::endl;
            }
        }

        // 2. 기본 데이터 검증
        FbxVector4* ctrlPoints = workingMesh->GetControlPoints();
        if (!ctrlPoints) {
            std::cout << "No control points found" << std::endl;
            return;
        }

        int polygonCount = workingMesh->GetPolygonCount();
        int controlPointCount = workingMesh->GetControlPointsCount();

        if (polygonCount <= 0 || controlPointCount <= 0) {
            std::cout << "Invalid mesh data" << std::endl;
            return;
        }

        std::cout << "Polygons: " << polygonCount << ", Control Points: " << controlPointCount << std::endl;

        // 3. 메모리 예약
        size_t estimatedSize = polygonCount * 4; // 여유 공간
        outVertices.reserve(outVertices.size() + estimatedSize);
        outIndices.reserve(outIndices.size() + estimatedSize);

        // 4. UV 세트 확인
        FbxStringList uvSetNames;
        workingMesh->GetUVSetNames(uvSetNames);
        bool hasUV = uvSetNames.GetCount() > 0;

        // 5. 폴리곤 처리 - 안전한 방식
        for (int polyIndex = 0; polyIndex < polygonCount; ++polyIndex)
        {
            int polySize = workingMesh->GetPolygonSize(polyIndex);

            // 모든 폴리곤을 삼각형으로 분할
            if (polySize >= 3)
            {
                // Fan triangulation - 첫 번째 정점을 중심으로
                for (int triIndex = 1; triIndex < polySize - 1; ++triIndex)
                {
                    int indices[3] = { 0, triIndex, triIndex + 1 };

                    for (int i = 0; i < 3; ++i)
                    {
                        ProcessSafeVertex(workingMesh, polyIndex, indices[i], hasUV, uvSetNames,
                            ctrlPoints, controlPointCount, outVertices, outIndices);
                    }
                }
            }

            // 진행 상황 출력
            if (polyIndex % 5000 == 0 && polyIndex > 0) {
                std::cout << "Processed " << polyIndex << "/" << polygonCount << " polygons..." << std::endl;
            }
        }

        std::cout << "Final mesh: " << outVertices.size() << " vertices, "
            << outIndices.size() << " indices (" << outIndices.size() / 3 << " triangles)" << std::endl;

    }
    catch (const std::exception& e) {
        std::cout << "Exception in ParseMesh: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception in ParseMesh" << std::endl;
    }
}

void FBXLoader::ProcessSafeVertex(FbxMesh* mesh, int polyIndex, int vertexIndex,
    bool hasUV, FbxStringList& uvSetNames,
    FbxVector4* ctrlPoints, int controlPointCount,
    std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
    try {
        // 1. Control Point 인덱스 가져오기
        int cpIndex = mesh->GetPolygonVertex(polyIndex, vertexIndex);
        if (cpIndex < 0 || cpIndex >= controlPointCount) {
            return; // 유효하지 않은 인덱스는 스킵
        }

        // 2. 위치 정보
        FbxVector4 pos = ctrlPoints[cpIndex];

        Vertex vertex = {};
        vertex.position = XMFLOAT3(
            static_cast<float>(pos[0]),
            static_cast<float>(pos[1]),
            static_cast<float>(pos[2])
        );

        // 3. 노멀 정보 (안전하게)
        FbxVector4 normal(0, 1, 0, 0); // 기본값
        if (mesh->GetPolygonVertexNormal(polyIndex, vertexIndex, normal)) {
            normal.Normalize();
            vertex.normal = XMFLOAT3(
                static_cast<float>(normal[0]),
                static_cast<float>(normal[1]),
                static_cast<float>(normal[2])
            );
        }
        else {
            vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        }

        // 4. UV 정보 (안전하게)
        vertex.uv = XMFLOAT2(0.0f, 0.0f); // 기본값
        if (hasUV) {
            FbxVector2 uv;
            bool unmapped = false;
            if (mesh->GetPolygonVertexUV(polyIndex, vertexIndex, uvSetNames[0], uv, unmapped)) {
                if (!unmapped) {
                    vertex.uv = XMFLOAT2(
                        static_cast<float>(uv[0]),
                        1.0f - static_cast<float>(uv[1]) // DirectX Y-flip
                    );
                }
            }
        }

        // 5. 정점 추가 (중복 제거 없이 단순하게)
        outVertices.push_back(vertex);
        outIndices.push_back(static_cast<UINT>(outVertices.size() - 1));

    }
    catch (...) {
        // 개별 정점 처리 실패는 무시하고 계속 진행
        std::cout << "Failed to process vertex " << vertexIndex << " in polygon " << polyIndex << std::endl;
    }
}

void FBXLoader::ProcessNode(FbxNode* node, std::vector<Vertex>& outVertices, std::vector<UINT>& outIndices)
{
    if (!node) return;

    // 현재 노드의 메쉬 처리
    FbxMesh* mesh = node->GetMesh();
    if (mesh)
    {
        std::cout << "Processing mesh: " << node->GetName() << std::endl;
        ParseMesh(mesh, outVertices, outIndices);
    }

    // 자식 노드들 재귀 처리
    for (int i = 0; i < node->GetChildCount(); ++i)
    {
        ProcessNode(node->GetChild(i), outVertices, outIndices);
    }
}