#include "pch.h"
#include "Importer.h"

bool Importer::LoadModel(const wstring& basePath)
{
    Release();

    wstring meshPath = basePath + L"_0.mesh";
    if (!LoadMesh(meshPath)) {
        MASSERT(false, "Failed to load mesh file");
        return false;
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

    meshData.indices.resize(header.indexCount);
    ifs.read(reinterpret_cast<char*>(meshData.indices.data()),
        sizeof(UINT) * header.indexCount);

    meshData.hasAnimation = (header.hasAnimation == 1);

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

        BoneData& bone = skeletonData.bones[i];
        bone.name = string(boneData.name);
        bone.parentIndex = boneData.parentIndex;
        ConvertFloat4x4ToMatrix(boneData.offsetMatrix, bone.offsetMatrix);
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

        if (header.magic != 'MINA') { 
            continue;
        }

        AnimationData animData;
        animData.name = string(header.name);
        animData.duration = header.duration;
        animData.boneKeyFrames.resize(header.boneCount);

        for (uint32_t boneIdx = 0; boneIdx < header.boneCount; ++boneIdx) {
            animData.boneKeyFrames[boneIdx].resize(header.frameCount);

            for (uint32_t frameIdx = 0; frameIdx < header.frameCount; ++frameIdx) {
                KeyFrameData& keyFrame = animData.boneKeyFrames[boneIdx][frameIdx];

                ifs.read(reinterpret_cast<char*>(&keyFrame.time), sizeof(float));

                float matrix[16];
                ifs.read(reinterpret_cast<char*>(matrix), sizeof(matrix));
                ConvertFloat4x4ToMatrix(matrix, keyFrame.transform);
            }
        }

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