#include "pch.h"
#include "Exporter.h"

bool Exporter::ExportAll(FBXLoader& loader, const wstring& basePath, const wstring& fbxDir)
{
    // 경로 정보 추출
    wstring outputDir = basePath.substr(0, basePath.find_last_of(L"/\\"));
    if (outputDir.empty()) outputDir = L".";  // 현재 디렉토리

    // 1. 메시 저장
    for (int i = 0; i < loader.GetMeshCount(); ++i) {
        wstring meshPath = basePath + L"_" + to_wstring(i) + L".mesh";
        if (!ExportMesh(loader.GetMesh(i), meshPath)) return false;
    }

    // 2. 스켈레톤 저장
    if (!loader.GetBones().empty()) {
        if (!ExportSkeleton(loader.GetBones(), basePath + L".skel")) return false;
    }

    // 3. 애니메이션 저장
    for (auto& animClip : loader.GetAnimClip()) {
        wstring animName = animClip->name;
        replace(animName.begin(), animName.end(), L'|', L'_');
        wstring animPath = basePath + L"_" + animName + L".anim";
        if (!ExportAnimation(*animClip, animPath)) return false;
    }

    // 4. 머티리얼 + 텍스처 저장
    if (loader.GetMeshCount() > 0 && !loader.GetMesh(0).materials.empty()) {
        if (!ExportMaterials(loader.GetMesh(0).materials, basePath + L".mtl")) return false;
        if (!ProcessTextures(loader.GetMesh(0).materials, fbxDir, outputDir)) return false;
    }

    return true;
}

bool Exporter::ExportMesh(const FbxMeshInfo& meshInfo, const wstring& path)
{
    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"메시 파일 생성 실패: " << path << endl;
        return false;
    }

    // 헤더 작성
    MeshBinaryHeader header = {};
    header.magic = 'HSEM';  // 'MESH' 역순 (little endian)
    header.vertexCount = static_cast<uint32_t>(meshInfo.vertices.size());

    // 인덱스 개수 계산 (모든 서브메시 합계)
    uint32_t totalIndices = 0;
    for (const auto& indices : meshInfo.indices) {
        totalIndices += static_cast<uint32_t>(indices.size());
    }
    header.indexCount = totalIndices;
    header.materialCount = static_cast<uint32_t>(meshInfo.materials.size());
    header.hasAnimation = meshInfo.hasAnimation ? 1 : 0;

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // 정점 데이터 작성
    ofs.write(reinterpret_cast<const char*>(meshInfo.vertices.data()),
        sizeof(Vertex) * meshInfo.vertices.size());

    // 인덱스 데이터 작성 (모든 서브메시를 하나로 합침)
    for (const auto& indices : meshInfo.indices) {
        ofs.write(reinterpret_cast<const char*>(indices.data()),
            sizeof(uint32_t) * indices.size());
    }

    wcout << L"메시 저장 완료: " << path << endl;
    wcout << L"  정점: " << header.vertexCount << L", 인덱스: " << header.indexCount << endl;
    return true;
}

bool Exporter::ExportSkeleton(const vector<shared_ptr<FbxBoneInfo>>& bones, const wstring& path)
{
    if (bones.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"스켈레톤 파일 생성 실패: " << path << endl;
        return false;
    }

    // 헤더 작성
    SkeletonBinaryHeader header = {};
    header.magic = 'LEKS';  // 'SKEL' 역순
    header.boneCount = static_cast<uint32_t>(bones.size());

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // 본 데이터 작성
    for (const auto& bone : bones) {
        BoneBinaryData boneData = {};

        // 본 이름 복사
        string boneName = ws2s(bone->boneName);
        strncpy_s(boneData.name, boneName.c_str(), sizeof(boneData.name) - 1);

        boneData.parentIndex = bone->parentIndex;

        // FbxAMatrix를 float 배열로 변환
        ConvertFbxMatrixToFloat4x4(bone->matOffset, boneData.offsetMatrix);

        ofs.write(reinterpret_cast<const char*>(&boneData), sizeof(boneData));
    }

    wcout << L"스켈레톤 저장 완료: " << path << endl;
    wcout << L"  본 개수: " << header.boneCount << endl;
    return true;
}

bool Exporter::ExportAnimation(const FbxAnimClipInfo& animClip, const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"애니메이션 파일 생성 실패: " << path << endl;
        return false;
    }

    // 헤더 작성
    AnimationBinaryHeader header = {};
    header.magic = 'MINA';  // 'ANIM' 역순
    header.boneCount = static_cast<uint32_t>(animClip.keyFrames.size());

    // 프레임 개수 (첫 번째 본의 키프레임 개수로 가정)
    header.frameCount = animClip.keyFrames.empty() ? 0 :
        static_cast<uint32_t>(animClip.keyFrames[0].size());

    // 지속 시간 계산
    if (header.frameCount > 0) {
        const auto& firstBone = animClip.keyFrames[0];
        header.duration = static_cast<float>(firstBone.back().time - firstBone.front().time);
    }
    else {
        header.duration = 0.0f;
    }

    // 애니메이션 이름
    string animName = ws2s(animClip.name);
    strncpy_s(header.name, animName.c_str(), sizeof(header.name) - 1);

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // 키프레임 데이터 작성 (본별로)
    for (const auto& boneKeyFrames : animClip.keyFrames) {
        for (const auto& keyFrame : boneKeyFrames) {
            // 시간 정보
            float time = static_cast<float>(keyFrame.time);
            ofs.write(reinterpret_cast<const char*>(&time), sizeof(time));

            // 변환 행렬
            float matrix[16];
            ConvertFbxMatrixToFloat4x4(keyFrame.matTransform, matrix);
            ofs.write(reinterpret_cast<const char*>(matrix), sizeof(matrix));
        }
    }

    wcout << L"애니메이션 저장 완료: " << path << endl;
    wcout << L"  본: " << header.boneCount << L", 프레임: " << header.frameCount << endl;
    return true;
}

bool Exporter::ExportMaterials(const vector<FbxMaterialInfo>& materials, const wstring& path)
{
    if (materials.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"머티리얼 파일 생성 실패: " << path << endl;
        return false;
    }

    // 헤더 작성
    MaterialBinaryHeader header = {};
    header.magic = 'LTAM';  // 'MATL' 역순
    header.materialCount = static_cast<uint32_t>(materials.size());

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // 머티리얼 데이터 작성
    for (const auto& mat : materials) {
        MaterialBinaryData matData = {};
        
        // 이름과 색상 정보
        string name = ws2s(mat.name);
        strncpy_s(matData.name, name.c_str(), sizeof(matData.name) - 1);
        matData.diffuse = mat.diffuse;
        matData.ambient = mat.ambient;
        matData.specular = mat.specular;
        
        // 모든 텍스처 경로 저장
        auto saveTexturePath = [&](const wstring& texName, char* destPath, size_t destSize) {
            if (!texName.empty()) {
                wstring texFile = GetRelativeTexturePath(texName);
                string texPath = "textures/" + ws2s(texFile);
                strncpy_s(destPath, destSize, texPath.c_str(), destSize - 1);
            }
        };
        
        saveTexturePath(mat.baseColorTexName, matData.baseColorTexPath, sizeof(matData.baseColorTexPath));
        saveTexturePath(mat.normalTexName, matData.normalTexPath, sizeof(matData.normalTexPath));
        saveTexturePath(mat.roughnessTexName, matData.roughnessTexPath, sizeof(matData.roughnessTexPath));
        saveTexturePath(mat.metallicTexName, matData.metallicTexPath, sizeof(matData.metallicTexPath));
        saveTexturePath(mat.heightTexName, matData.heightTexPath, sizeof(matData.heightTexPath));
        saveTexturePath(mat.alphaTexName, matData.alphaTexPath, sizeof(matData.alphaTexPath));
        saveTexturePath(mat.emissionTexName, matData.emissionTexPath, sizeof(matData.emissionTexPath));
        saveTexturePath(mat.aoTexName, matData.aoTexPath, sizeof(matData.aoTexPath));
        
        ofs.write(reinterpret_cast<const char*>(&matData), sizeof(matData));
    }

    wcout << L"머티리얼 저장 완료: " << path << endl;
    wcout << L"  머티리얼 개수: " << header.materialCount << endl;
    return true;
}

bool Exporter::ProcessTextures(const vector<FbxMaterialInfo>& materials,
    const wstring& fbxDir, const wstring& outputDir)
{
    // textures 폴더 생성
    wstring textureDir = outputDir + L"/textures";
    CreateDirectoryW(textureDir.c_str(), nullptr);

    int copiedCount = 0;

    auto copyTexture = [&](const wstring& texName) {
        if (!texName.empty()) {
            wstring sourcePath = fbxDir + L"/" + texName;
            wstring fileName = GetRelativeTexturePath(texName);
            wstring destPath = textureDir + L"/" + fileName;
            if (CopyTextureFile(sourcePath, destPath)) copiedCount++;
        }
        };

    for (const auto& mat : materials) {
        copyTexture(mat.baseColorTexName);
        copyTexture(mat.normalTexName);
        copyTexture(mat.roughnessTexName);
        copyTexture(mat.metallicTexName);
        copyTexture(mat.heightTexName);
        copyTexture(mat.alphaTexName);
        copyTexture(mat.emissionTexName);
        copyTexture(mat.aoTexName);
    }

    wcout << L"텍스처 복사 완료: " << copiedCount << L"개 파일" << endl;
    return true;
}

void Exporter::ConvertFbxMatrixToFloat4x4(const FbxAMatrix& fbxMatrix, float matrix[16])
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            matrix[i * 4 + j] = static_cast<float>(fbxMatrix.mData[i][j]);
        }
    }
}

wstring Exporter::GetRelativeTexturePath(const wstring& textureName)
{
    // 경로에서 파일명만 추출
    size_t pos = textureName.find_last_of(L"/\\");
    if (pos != wstring::npos) {
        return textureName.substr(pos + 1);
    }
    return textureName;
}

bool Exporter::CopyTextureFile(const wstring& sourcePath, const wstring& destPath)
{
    // 파일 존재 확인
    DWORD attributes = GetFileAttributesW(sourcePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        wcout << L"텍스처 파일 없음: " << sourcePath << endl;
        return false;
    }

    // 파일 복사
    if (CopyFileW(sourcePath.c_str(), destPath.c_str(), FALSE)) {
        wcout << L"텍스처 복사: " << GetRelativeTexturePath(sourcePath) << endl;
        return true;
    }
    else {
        wcout << L"텍스처 복사 실패: " << sourcePath << endl;
        return false;
    }
}
