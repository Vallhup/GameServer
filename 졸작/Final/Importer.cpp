#include "pch.h"
#include "Importer.h"

bool Importer::LoadModel(const wstring& basePath)
{
    Release();

    for (int i = 0; i < 9; ++i) {  // 충분한 범위로 검색
        wstring meshPath = basePath + L"_" + to_wstring(i) + L".mesh";

        ifstream testFile(meshPath);
        if (!testFile.good()) {
            if (i == 0) {
                MASSERT(false, "No mesh files found");
                return false;
            }
            break;  // 더 이상 파일이 없으면 종료
        }
        testFile.close();

        if (!LoadAndMergeMesh(meshPath)) {
            OutputDebugStringA(("Failed to load: " + string(meshPath.begin(), meshPath.end()) + "\n").c_str());
        }
    }

    wstring materialPath = basePath + L".mtl";
    if (!LoadMaterials(materialPath)) {
        MASSERT(false, "Failed to load material file");
        return false;
    }

    wstring skeletonPath = basePath + L".skel";
    if (!LoadSkeleton(skeletonPath)) {
        OutputDebugStringA("Warning: Failed to load skeleton file\n");
    }
    
    if (!LoadAnimations(basePath)) {
        OutputDebugStringA("Warning: Failed to load animation files\n");
    }
    
    return true;
}

void Importer::Release()
{
    meshData = {};
    skeletonData = {};
    animationData.clear();
    materialData.clear();
}

bool Importer::LoadMesh(const wstring& path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs) {
        OutputDebugStringA("Failed to open mesh file\n");
        return false;
    }

    MeshBinaryHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 'HSEM') { 
        OutputDebugStringA("Invalid mesh file magic\n");
        return false;
    }

    meshData.vertices.resize(header.vertexCount);
    ifs.read(reinterpret_cast<char*>(meshData.vertices.data()),
        sizeof(Vertex) * header.vertexCount);

    meshData.subMeshes.resize(header.subMeshCount);
    ifs.read(reinterpret_cast<char*>(meshData.subMeshes.data()),
        sizeof(SubMeshInfo) * header.subMeshCount);

    meshData.indices.resize(header.indexCount);
    ifs.read(reinterpret_cast<char*>(meshData.indices.data()),
        sizeof(UINT) * header.indexCount);

    meshData.hasAnimation = (header.hasAnimation == 1);

    return true;
}

bool Importer::LoadAndMergeMesh(const wstring& path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs) {
        OutputDebugStringA(("Failed to open: " + string(path.begin(), path.end()) + "\n").c_str());
        return false;
    }

    MeshBinaryHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 'HSEM') {
        OutputDebugStringA(("Invalid magic in: " + string(path.begin(), path.end()) + "\n").c_str());
        return false;
    }

    if (header.vertexCount == 0 || header.indexCount == 0) {
        OutputDebugStringA(("Empty mesh: " + string(path.begin(), path.end()) + "\n").c_str());
        return true;
    }

    vector<Vertex> tempVertices(header.vertexCount);
    ifs.read(reinterpret_cast<char*>(tempVertices.data()),
        sizeof(Vertex) * header.vertexCount);

    vector<SubMeshInfo> tempSubMeshes(header.subMeshCount);
    ifs.read(reinterpret_cast<char*>(tempSubMeshes.data()),
        sizeof(SubMeshInfo) * header.subMeshCount);

    vector<UINT> tempIndices(header.indexCount);
    ifs.read(reinterpret_cast<char*>(tempIndices.data()),
        sizeof(UINT) * header.indexCount);

    UINT vertexOffset = static_cast<UINT>(meshData.vertices.size());
    UINT indexOffset = static_cast<UINT>(meshData.indices.size());

    // 버텍스 추가
    meshData.vertices.insert(meshData.vertices.end(),
        tempVertices.begin(), tempVertices.end());

    // 인덱스 추가 (오프셋 보정)
    for (auto& idx : tempIndices) {
        meshData.indices.push_back(idx + vertexOffset);
    }

    // *** 핵심: 각 메시의 SubMesh에 고유한 머티리얼 인덱스 할당 ***
    for (auto& subMesh : tempSubMeshes) {
        subMesh.startIndex += indexOffset;

        // 충돌체 메시들에게 머티리얼 인덱스 0 할당 (기본 머티리얼)
        subMesh.materialIndex = 0;

        meshData.subMeshes.push_back(subMesh);
    }

    if (vertexOffset == 0) {
        meshData.hasAnimation = (header.hasAnimation == 1);
    }

    OutputDebugStringA(("Merged: " + string(path.begin(), path.end()) +
        " - Vertices: " + to_string(tempVertices.size()) +
        ", Indices: " + to_string(tempIndices.size()) + "\n").c_str());
    return true;
}

bool Importer::LoadSkeleton(const wstring& path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs) {
        OutputDebugStringA("Failed to open skeleton file\n");
        return false;
    }

    SkeletonBinaryHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 'LEKS') { 
        OutputDebugStringA("Invalid skeleton file magic\n");
        return false;
    }

    skeletonData.bones.resize(header.boneCount);
    for (uint32_t i = 0; i < header.boneCount; ++i) {
        BoneBinaryData boneData;
        ifs.read(reinterpret_cast<char*>(&boneData), sizeof(boneData));

        BoneInfo& bone = skeletonData.bones[i];
        bone.boneName = string(boneData.name);
        bone.parentIdx = boneData.parentIndex;
        ConvertFloat4x4ToMatrix(boneData.offsetMatrix, bone.matOffset);
    }

    return true;
}

bool Importer::LoadAnimations(const wstring& basePath)
{
    wstring searchPath = basePath + L"_*.anim";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("No animation files found\n");
        return false;
    }

    wstring directory = basePath.substr(0, basePath.find_last_of(L"/\\") + 1);

    do {
        wstring animPath = directory + findData.cFileName;
        ifstream ifs(animPath, ios::binary);
        if (!ifs) continue;

        AnimationBinaryHeader header;
        ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (header.magic != 'MINA') continue;

        AnimClipInfo animData;  // 구조 변경
        animData.animName = string(header.name);
        animData.duration = header.duration;
        animData.frameCount = header.frameCount;

        // 레퍼런스와 동일한 구조로 로드: [frameIndex * boneCount + boneIndex]
        size_t totalFrames = header.boneCount * header.frameCount;
        animData.keyFrames.resize(totalFrames);

        ifs.read(reinterpret_cast<char*>(animData.keyFrames.data()),
            totalFrames * sizeof(AnimFrameParams));

        animationData.push_back(animData);

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return !animationData.empty();
}

bool Importer::LoadMaterials(const wstring& path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs) {
        OutputDebugStringA("Failed to open material file\n");
        return false;
    }

    MaterialBinaryHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 'LTAM') {
        OutputDebugStringA("Invalid material file magic\n");
        return false;
    }

    materialData.resize(header.materialCount);
    for (uint32_t i = 0; i < header.materialCount; ++i) {
        MaterialBinaryData matData;
        ifs.read(reinterpret_cast<char*>(&matData), sizeof(matData));

        MaterialData& material = materialData[i];
        material.name = string(matData.name);
        material.diffuse = matData.diffuse;
        material.ambient = matData.ambient;
        material.specular = matData.specular;

        material.baseColorTexPath = string(matData.baseColorTexPath);
        material.normalTexPath = string(matData.normalTexPath);
        material.roughnessTexPath = string(matData.roughnessTexPath);
        material.metallicTexPath = string(matData.metallicTexPath);
        material.heightTexPath = string(matData.heightTexPath);
        material.alphaTexPath = string(matData.alphaTexPath);
        material.emissionTexPath = string(matData.emissionTexPath);
        material.aoTexPath = string(matData.aoTexPath);
    }

    // *** 충돌체 메시용 추가 머티리얼들 생성 ***
    for (int i = 1; i <= 8; ++i) {  // _1~8.mesh용
        MaterialData collisionMat;
        collisionMat.name = "Collision_" + to_string(i);
        collisionMat.diffuse = { 1.0f, 0.0f, 0.0f, 1.0f };  // 빨간색
        collisionMat.ambient = { 1.0f, 0.0f, 0.0f, 1.0f };
        collisionMat.specular = { 0.0f, 0.0f, 0.0f, 1.0f };
        // 모든 텍스처 경로는 빈 문자열 (기본값)
        materialData.push_back(collisionMat);
    }

    OutputDebugStringA(("Total materials loaded: " + to_string(materialData.size()) + "\n").c_str());
    return true;
}

void Importer::ConvertFloat4x4ToMatrix(const float matrix[16], XMMATRIX& outMatrix)
{
    outMatrix = XMMATRIX(
        matrix[0], matrix[1], matrix[2], matrix[3],
        matrix[4], matrix[5], matrix[6], matrix[7],
        matrix[8], matrix[9], matrix[10], matrix[11],
        matrix[12], matrix[13], matrix[14], matrix[15]
    );
}