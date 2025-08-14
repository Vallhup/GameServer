#pragma once

struct AnimFrameParams
{
    XMFLOAT4 scale;
    XMFLOAT4 rotation;    // Quaternion
    XMFLOAT4 translation;
};

struct BoneInfo
{
    string boneName;
    int32_t parentIdx;
    XMMATRIX matOffset;
};

struct AnimClipInfo
{
    string animName;
    int32_t frameCount;
    float duration;
    vector<AnimFrameParams> keyFrames;  // [frameIndex * boneCount + boneIndex] ¼ø¼­
};

struct MeshBinaryHeader {
    uint32_t magic;           // 'MESH'
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t materialCount;
    uint32_t hasAnimation;
    uint32_t subMeshCount;
};

struct SubMeshInfo {
    uint32_t startIndex;
    uint32_t indexCount;
    uint32_t materialIndex;
};

struct SkeletonBinaryHeader {
    uint32_t magic;           // 'SKEL'
    uint32_t boneCount;
};

struct AnimationBinaryHeader {
    uint32_t magic;           // 'ANIM'
    uint32_t boneCount;
    uint32_t frameCount;
    float duration;
    char name[64];
};

struct MaterialBinaryHeader {
    uint32_t magic;           // 'MATL'
    uint32_t materialCount;
};

struct MaterialBinaryData {
    char name[64];
    XMFLOAT4 diffuse;
    XMFLOAT4 ambient;
    XMFLOAT4 specular;

    char baseColorTexPath[256];
    char normalTexPath[256];
    char roughnessTexPath[256];
    char metallicTexPath[256];
    char heightTexPath[256];
    char alphaTexPath[256];
    char emissionTexPath[256];
    char aoTexPath[256];
};

struct BoneBinaryData {
    char name[64];
    int32_t parentIndex;
    float offsetMatrix[16];
};

struct MeshData {
    vector<Vertex> vertices;
    vector<UINT> indices;
    vector<SubMeshInfo> subMeshes;
    bool hasAnimation = false;
};

struct SkeletonData {
    vector<BoneInfo> bones;
};

struct MaterialData {
    string name;
    XMFLOAT4 diffuse;
    XMFLOAT4 ambient;
    XMFLOAT4 specular;

    string baseColorTexPath;
    string normalTexPath;
    string roughnessTexPath;
    string metallicTexPath;
    string heightTexPath;
    string alphaTexPath;
    string emissionTexPath;
    string aoTexPath;
};

class Importer
{
public:
    bool LoadModel(const wstring& basePath);
    void Release();

    const MeshData& GetMesh() const { return meshData; }
    const SkeletonData& GetSkeleton() const { return skeletonData; }
    const vector<AnimClipInfo>& GetAnimations() const { return animationData; }
    const vector<MaterialData>& GetMaterials() const { return materialData; }

    bool HasAnimation() const { return meshData.hasAnimation; }

private:
    bool LoadMesh(const wstring& path);
    bool LoadSkeleton(const wstring& path);
    bool LoadAnimations(const wstring& basePath);
    bool LoadMaterials(const wstring& path);

    void ConvertFloat4x4ToMatrix(const float matrix[16], XMMATRIX& outMatrix);

private:
    MeshData meshData;
    SkeletonData skeletonData;
    vector<AnimClipInfo> animationData;
    vector<MaterialData> materialData;
};

