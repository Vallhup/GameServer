#pragma once
#include "FBXLoader.h"

// 바이너리 헤더 구조체들
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

// 개별 머티리얼 데이터
struct MaterialBinaryData {
    char name[64];
    Vec4 diffuse;
    Vec4 ambient;
    Vec4 specular;

    // 모든 텍스처 경로
    char baseColorTexPath[256];
    char normalTexPath[256];
    char roughnessTexPath[256];
    char metallicTexPath[256];
    char heightTexPath[256];
    char alphaTexPath[256];
    char emissionTexPath[256];
    char aoTexPath[256];
};

// 본 데이터 (FbxAMatrix를 float 배열로 변환)
struct BoneBinaryData {
    char name[64];
    int32_t parentIndex;
    float offsetMatrix[16];
};

class Exporter
{
public:
    // 메인 함수 - 모든 파일 한번에 내보내기
    bool ExportAll(FBXLoader& loader, const wstring& basePath, const wstring& fbxDir,
        const wstring& referenceSkeletonPath = L"");  // 기준 스켈레톤 경로 (애니메이션 매핑용)

private:
    // 개별 내보내기 함수들
    bool ExportMesh(const FbxMeshInfo& meshInfo, const wstring& path);
    bool ExportMeshAsText(const FbxMeshInfo& meshInfo, const wstring& path);
    bool ExportSkeleton(const vector<shared_ptr<FbxBoneInfo>>& bones, const wstring& path);
    bool ExportSkeletonText(const vector<shared_ptr<FbxBoneInfo>>& bones, const wstring& path);

    // 기존 (매핑 없이)
    bool ExportAnimation(const FbxAnimClipInfo& animClip, const wstring& path);
    bool ExportAnimationAsText(const FbxAnimClipInfo& animClip, const wstring& path);

    // 기준 스켈레톤 매핑 버전
    bool ExportAnimationWithMapping(
        const FbxAnimClipInfo& animClip,
        const vector<shared_ptr<FbxBoneInfo>>& animBones,
        const vector<shared_ptr<FbxBoneInfo>>& refBones,
        const wstring& path);
    bool ExportAnimationAsTextWithMapping(
        const FbxAnimClipInfo& animClip,
        const vector<shared_ptr<FbxBoneInfo>>& animBones,
        const vector<shared_ptr<FbxBoneInfo>>& refBones,
        const wstring& path);

    bool ExportMaterials(const vector<FbxMaterialInfo>& materials, const wstring& path);
    bool ProcessTextures(const vector<FbxMaterialInfo>& materials,
        const wstring& fbxDir, const wstring& outputDir);

    bool LoadSkeletonFromFile(const wstring& path, vector<shared_ptr<FbxBoneInfo>>& bones);

    bool ExportBakedAnimation(
        const vector<shared_ptr<FbxBoneInfo>>& animBones,
        const vector<shared_ptr<FbxBoneInfo>>& refBones,
        const FbxAnimClipInfo& animClip,
        const wstring& path
    );

    // 본 이름으로 애니메이션 본 인덱스 찾기
    int32_t FindAnimBoneIndex(const vector<shared_ptr<FbxBoneInfo>>& animBones, const wstring& boneName);

    // 유틸 함수들
    void ConvertFbxMatrixToFloat4x4(const FbxAMatrix& fbxMatrix, float matrix[16]);
    FbxAMatrix ApplyReflectionMatrix(const FbxAMatrix& matrix);
    wstring GetRelativeTexturePath(const wstring& textureName);
    bool CopyTextureFile(const wstring& sourcePath, const wstring& destPath);
};

