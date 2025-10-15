#include "pch.h"
#include "Exporter.h"

bool Exporter::ExportAll(FBXLoader& loader, const wstring& basePath, const wstring& fbxDir)
{
    wstring outputDir = basePath.substr(0, basePath.find_last_of(L"/\\"));
    if (outputDir.empty()) outputDir = L".";  

    for (int i = 0; i < loader.GetMeshCount(); ++i) {
        wstring meshPath = basePath + L"_" + to_wstring(i) + L".mesh";
        if (!ExportMesh(loader.GetMesh(i), meshPath)) return false;
    }

    if (!loader.GetBones().empty()) {
        if (!ExportSkeleton(loader.GetBones(), basePath + L".skel")) return false;
    }

    for (auto& animClip : loader.GetAnimClip()) {
        wstring animName = animClip->name;
        replace(animName.begin(), animName.end(), L'|', L'_');
        wstring animPath = basePath + L"_" + animName + L".anim";
        if (!ExportAnimation(*animClip, animPath)) return false;

        animPath = basePath + L"_Text_" + animName + L".anim";
        if (!ExportAnimationAsText(*animClip, animPath)) return false;
    }

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

    MeshBinaryHeader header = {};
    header.magic = 'HSEM';  
    header.vertexCount = static_cast<uint32_t>(meshInfo.vertices.size());

    uint32_t totalIndices = 0;
    for (const auto& indices : meshInfo.indices) {
        totalIndices += static_cast<uint32_t>(indices.size());
    }
    header.indexCount = totalIndices;
    header.materialCount = static_cast<uint32_t>(meshInfo.materials.size());
    header.hasAnimation = meshInfo.hasAnimation ? 1 : 0;
    header.subMeshCount = static_cast<uint32_t>(meshInfo.indices.size());

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    ofs.write(reinterpret_cast<const char*>(meshInfo.vertices.data()),
        sizeof(Vertex) * meshInfo.vertices.size());

    uint32_t currentOffset = 0;
    for (size_t i = 0; i < meshInfo.indices.size(); ++i) {
        SubMeshInfo subMesh = {};
        subMesh.startIndex = currentOffset;
        subMesh.indexCount = static_cast<uint32_t>(meshInfo.indices[i].size());
        subMesh.materialIndex = static_cast<uint32_t>(i);

        ofs.write(reinterpret_cast<const char*>(&subMesh), sizeof(subMesh));
        currentOffset += subMesh.indexCount;
    }

    for (const auto& indices : meshInfo.indices) {
        ofs.write(reinterpret_cast<const char*>(indices.data()),
            sizeof(uint32_t) * indices.size());
    }

    wcout << L"메시 저장 완료: " << path << endl;
    wcout << L"  정점: " << header.vertexCount << L", 인덱스: " << header.indexCount
        << L", 서브메시: " << header.subMeshCount << endl;
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

    SkeletonBinaryHeader header = {};
    header.magic = 'LEKS';  
    header.boneCount = static_cast<uint32_t>(bones.size());

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (const auto& bone : bones) {
        BoneBinaryData boneData = {};

        string boneName = ws2s(bone->boneName);
        strncpy_s(boneData.name, boneName.c_str(), sizeof(boneData.name) - 1);

        boneData.parentIndex = bone->parentIndex;

        ConvertFbxMatrixToFloat4x4(bone->matOffset, boneData.offsetMatrix);

        ofs.write(reinterpret_cast<const char*>(&boneData), sizeof(boneData));
    }

    wcout << L"스켈레톤 저장 완료: " << path << endl;
    wcout << L"  본 개수: " << header.boneCount << endl;
    return true;
}

bool Exporter::ExportSkeletonText(const vector<shared_ptr<FbxBoneInfo>>& bones, const wstring& path)
{
    if (bones.empty()) return true;

    wofstream ofs(path);
    if (!ofs) {
        wcout << L"스켈레톤 파일 생성 실패: " << path << endl;
        return false;
    }

    SkeletonBinaryHeader header = {};
    header.magic = 'LEKS';
    header.boneCount = static_cast<uint32_t>(bones.size());

    ofs << header.magic << endl;
    ofs << L"BoneCount: " << header.boneCount << endl;
    ofs << L"------------------------------" << endl;

    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];

        ofs << L"Bone[" << i << L"]" << endl;
        ofs << L"  Name: " << bone->boneName << endl;
        ofs << L"  ParentIndex: " << bone->parentIndex << endl;
        ofs << L"  OffsetMatrix: " << endl;

        float matrix[16];
        ConvertFbxMatrixToFloat4x4(bone->matOffset, matrix);

        for (int row = 0; row < 4; ++row) {
            ofs << L"  ";
            for (int col = 0; col < 4; ++col) {
                float v = matrix[row * 4 + col];
                ofs << v;
                ofs << L"  ";
            }
            ofs << endl;
        }
        ofs << endl;
    }
}

// ExportAnimation 함수 수정 - FBX Quaternion 타입 문제 해결
bool Exporter::ExportAnimation(const FbxAnimClipInfo& animClip, const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"애니메이션 파일 생성 실패: " << path << endl;
        return false;
    }

    AnimationBinaryHeader header = {};
    header.magic = 'MINA';
    header.boneCount = static_cast<uint32_t>(animClip.keyFrames.size());

    // 키프레임이 있는 첫 번째 본에서 frameCount 가져오기
    header.frameCount = 0;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            header.frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    // duration 계산
    if (header.frameCount > 0) {
        for (const auto& boneFrames : animClip.keyFrames) {
            if (!boneFrames.empty()) {
                header.duration = static_cast<float>(boneFrames.back().time - boneFrames.front().time);
                break;
            }
        }
    }
    else {
        header.duration = 0.0f;
    }

    string animName = ws2s(animClip.name);
    strncpy_s(header.name, animName.c_str(), sizeof(header.name) - 1);
    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // ★ 기존 프로젝트와 동일한 레이아웃으로 저장: [frame * boneCount + bone]
    for (uint32_t frameIdx = 0; frameIdx < header.frameCount; ++frameIdx) {
        for (uint32_t boneIdx = 0; boneIdx < header.boneCount; ++boneIdx) {

            if (frameIdx < animClip.keyFrames[boneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[boneIdx][frameIdx];

                // SQT 분해
                FbxVector4 scale = keyFrame.matTransform.GetS();
                FbxQuaternion rotation = keyFrame.matTransform.GetQ();
                FbxVector4 translation = keyFrame.matTransform.GetT();

                struct AnimFrameParams {
                    float scale[4];
                    float rotation[4];
                    float translation[4];
                } params;

                params.scale[0] = static_cast<float>(scale[0]);
                params.scale[1] = static_cast<float>(scale[1]);
                params.scale[2] = static_cast<float>(scale[2]);
                params.scale[3] = 1.0f;

                params.rotation[0] = static_cast<float>(rotation[0]);
                params.rotation[1] = static_cast<float>(rotation[1]);
                params.rotation[2] = static_cast<float>(rotation[2]);
                params.rotation[3] = static_cast<float>(rotation[3]);

                params.translation[0] = static_cast<float>(translation[0]);
                params.translation[1] = static_cast<float>(translation[1]);
                params.translation[2] = static_cast<float>(translation[2]);
                params.translation[3] = 0.0f;

                ofs.write(reinterpret_cast<const char*>(&params), sizeof(params));
            }
            else {
                // 프레임이 없으면 기본값
                struct AnimFrameParams {
                    float scale[4] = { 1,1,1,1 };
                    float rotation[4] = { 0,0,0,1 };
                    float translation[4] = { 0,0,0,0 };
                } params;

                ofs.write(reinterpret_cast<const char*>(&params), sizeof(params));
            }
        }
    }

    wcout << L"애니메이션 저장 완료: " << path << endl;
    return true;
}

bool Exporter::ExportAnimationAsText(const FbxAnimClipInfo& animClip, const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path);
    if (!ofs) {
        wcout << L"애니메이션 파일 생성 실패: " << path << endl;
        return false;
    }

    AnimationBinaryHeader header = {};
    header.magic = 'MINA';
    header.boneCount = static_cast<uint32_t>(animClip.keyFrames.size());

    // 키프레임이 있는 첫 번째 본에서 frameCount 가져오기
    header.frameCount = 0;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            header.frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    // duration 계산
    if (header.frameCount > 0) {
        for (const auto& boneFrames : animClip.keyFrames) {
            if (!boneFrames.empty()) {
                header.duration = static_cast<float>(boneFrames.back().time - boneFrames.front().time);
                break;
            }
        }
    }
    else {
        header.duration = 0.0f;
    }

    string animName = ws2s(animClip.name);
    strncpy_s(header.name, animName.c_str(), sizeof(header.name) - 1);

    ofs << "magic: " << header.magic << endl;
    ofs << "boneCount: " << header.boneCount << endl;
    ofs << "frameCount: " << header.frameCount << endl;
    ofs << "duration: " << header.duration << endl;
    ofs << "name: " << header.name << endl;
    ofs << "------------------------------" << endl;

    for (uint32_t frameIdx = 0; frameIdx < header.frameCount; ++frameIdx) {
        for (uint32_t boneIdx = 0; boneIdx < header.boneCount; ++boneIdx) {

            if (frameIdx < animClip.keyFrames[boneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[boneIdx][frameIdx];

                FbxVector4 scale = keyFrame.matTransform.GetS();
                FbxQuaternion rotation = keyFrame.matTransform.GetQ();
                FbxVector4 translation = keyFrame.matTransform.GetT();

                ofs << "Frame[" << frameIdx << "] Bone[" << boneIdx << "]" << endl;

                ofs << "  scale: " << static_cast<float>(scale[0]) << " "
                    << static_cast<float>(scale[1]) << " "
                    << static_cast<float>(scale[2]) << " "
                    << 1.0f << endl;

                ofs << "  rotation: " << static_cast<float>(rotation[0]) << " "
                    << static_cast<float>(rotation[1]) << " "
                    << static_cast<float>(rotation[2]) << " "
                    << static_cast<float>(rotation[3]) << endl;

                ofs << "  translation: " << static_cast<float>(translation[0]) << " "
                    << static_cast<float>(translation[1]) << " "
                    << static_cast<float>(translation[2]) << " "
                    << 0.0f << endl;
            }
            else {
                ofs << "Frame[" << frameIdx << "] Bone[" << boneIdx << "]" << endl;
                ofs << "  scale: 1 1 1 1" << endl;
                ofs << "  rotation: 0 0 0 1" << endl;
                ofs << "  translation: 0 0 0 0" << endl;
            }
        }
    }

    wcout << L"애니메이션 저장 완료: " << path << endl;
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

    MaterialBinaryHeader header = {};
    header.magic = 'LTAM';  
    header.materialCount = static_cast<uint32_t>(materials.size());

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (const auto& mat : materials) {
        MaterialBinaryData matData = {};
        
        string name = ws2s(mat.name);
        strncpy_s(matData.name, name.c_str(), sizeof(matData.name) - 1);
        matData.diffuse = mat.diffuse;
        matData.ambient = mat.ambient;
        matData.specular = mat.specular;
        
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

FbxAMatrix Exporter::ApplyReflectionMatrix(const FbxAMatrix& matrix)
{
    // 루키스와 동일한 reflection 행렬 (Y-Z 축 스왑)
    FbxVector4 v1 = { 1, 0, 0, 0 };
    FbxVector4 v2 = { 0, 0, 1, 0 };  // Y-Z 스왑
    FbxVector4 v3 = { 0, 1, 0, 0 };
    FbxVector4 v4 = { 0, 0, 0, 1 };

    FbxAMatrix matReflect;
    matReflect.mData[0] = v1;
    matReflect.mData[1] = v2;
    matReflect.mData[2] = v3;
    matReflect.mData[3] = v4;

    // reflection * matrix * reflection 적용 (루키스 방식)
    return matReflect * matrix * matReflect;
}

wstring Exporter::GetRelativeTexturePath(const wstring& textureName)
{
    size_t pos = textureName.find_last_of(L"/\\");
    if (pos != wstring::npos) {
        return textureName.substr(pos + 1);
    }
    return textureName;
}

bool Exporter::CopyTextureFile(const wstring& sourcePath, const wstring& destPath)
{
    DWORD attributes = GetFileAttributesW(sourcePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        wcout << L"텍스처 파일 없음: " << sourcePath << endl;
        return false;
    }

    if (CopyFileW(sourcePath.c_str(), destPath.c_str(), FALSE)) {
        wcout << L"텍스처 복사: " << GetRelativeTexturePath(sourcePath) << endl;
        return true;
    }
    else {
        wcout << L"텍스처 복사 실패: " << sourcePath << endl;
        return false;
    }
}
