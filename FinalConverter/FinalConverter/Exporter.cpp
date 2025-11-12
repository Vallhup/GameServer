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

        wstring bakedPath = basePath + L"_" + animName + L"_baked.bone";
        if (!ExportBakedAnimation(loader.GetBones(), *animClip, bakedPath))
            return false;
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

bool Exporter::ExportMeshAsText(const FbxMeshInfo& meshInfo, const wstring& path)
{
    wofstream ofs(path);
    if (!ofs) {
        wcout << L"메시 텍스트 파일 생성 실패: " << path << endl;
        return false;
    }

    // 헤더 정보
    ofs << L"MESH_TEXT" << endl;
    ofs << L"MeshName: " << meshInfo.name << endl;
    ofs << L"VertexCount: " << meshInfo.vertices.size() << endl;

    uint32_t totalIndices = 0;
    for (const auto& indices : meshInfo.indices) {
        totalIndices += static_cast<uint32_t>(indices.size());
    }
    ofs << L"IndexCount: " << totalIndices << endl;
    ofs << L"MaterialCount: " << meshInfo.materials.size() << endl;
    ofs << L"HasAnimation: " << (meshInfo.hasAnimation ? 1 : 0) << endl;
    ofs << L"SubMeshCount: " << meshInfo.indices.size() << endl;
    ofs << L"---" << endl;

    // 정점 데이터
    ofs << L"\n[VERTICES]" << endl;
    for (size_t i = 0; i < meshInfo.vertices.size(); ++i) {
        const auto& v = meshInfo.vertices[i];

        ofs << L"Vertex[" << i << L"]" << endl;
        ofs << L"  Position: " << v.pos.x << L" " << v.pos.y << L" " << v.pos.z << endl;
        ofs << L"  Normal: " << v.normal.x << L" " << v.normal.y << L" " << v.normal.z << endl;
        ofs << L"  UV: " << v.uv.x << L" " << v.uv.y << endl;
        ofs << L"  Tangent: " << v.tangent.x << L" " << v.tangent.y << L" " << v.tangent.z << endl;

        // 애니메이션 가중치 (있는 경우)
        if (meshInfo.hasAnimation) {
            ofs << L"  BoneIndices: " << v.indices.x << L" " << v.indices.y << L" "
                << v.indices.z << L" " << v.indices.w << endl;
            ofs << L"  BoneWeights: " << v.weights.x << L" " << v.weights.y << L" "
                << v.weights.z << L" " << v.weights.w << endl;
        }
    }

    // 서브메시 정보
    ofs << L"\n[SUBMESHES]" << endl;
    uint32_t currentOffset = 0;
    for (size_t i = 0; i < meshInfo.indices.size(); ++i) {
        ofs << L"SubMesh[" << i << L"]" << endl;
        ofs << L"  StartIndex: " << currentOffset << endl;
        ofs << L"  IndexCount: " << meshInfo.indices[i].size() << endl;
        ofs << L"  MaterialIndex: " << i << endl;

        currentOffset += static_cast<uint32_t>(meshInfo.indices[i].size());
    }

    // 인덱스 데이터
    ofs << L"\n[INDICES]" << endl;
    for (size_t subMeshIdx = 0; subMeshIdx < meshInfo.indices.size(); ++subMeshIdx) {
        ofs << L"SubMesh[" << subMeshIdx << L"] Indices:" << endl;

        const auto& indices = meshInfo.indices[subMeshIdx];

        // 삼각형 단위로 출력 (가독성)
        for (size_t i = 0; i < indices.size(); i += 3) {
            ofs << L"  Triangle[" << (i / 3) << L"]: ";
            ofs << indices[i] << L" " << indices[i + 1] << L" " << indices[i + 2] << endl;
        }
    }

    wcout << L"메시 텍스트 저장 완료: " << path << endl;
    wcout << L"  정점: " << meshInfo.vertices.size() << L", 인덱스: " << totalIndices
        << L", 서브메시: " << meshInfo.indices.size() << endl;

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

bool Exporter::LoadSkeletonFromFile(const wstring& path, vector<shared_ptr<FbxBoneInfo>>& bones)
{
    ifstream ifs(path, ios::binary);
    if (!ifs)
        return false;

    SkeletonBinaryHeader header = {};
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 'LEKS') {
        wcout << L"Invalid skeleton file: " << path << endl;
        return false;
    }

    bones.clear();
    bones.reserve(header.boneCount);

    for (uint32_t i = 0; i < header.boneCount; ++i) {
        BoneBinaryData boneData = {};
        ifs.read(reinterpret_cast<char*>(&boneData), sizeof(boneData));

        auto bone = make_shared<FbxBoneInfo>();
        bone->boneName = s2ws(string(boneData.name));
        bone->parentIndex = boneData.parentIndex;

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                bone->matOffset.mData[row][col] = boneData.offsetMatrix[row * 4 + col];
            }
        }

        bones.push_back(bone);
    }
}

bool Exporter::ExportBakedAnimation(const vector<shared_ptr<FbxBoneInfo>>& bones, const FbxAnimClipInfo& animClip, const wstring& path)
{
    if (animClip.keyFrames.empty()) {
        wcout << L"베이킹할 데이터 없음: " << path << endl;
        return true;
    }

    // boss.skel
    wstring skeletonPath = path.substr(0, path.find(L"_")) + L".skel";

    vector<shared_ptr<FbxBoneInfo>> baseSkeleton;
    if (!LoadSkeletonFromFile(skeletonPath, baseSkeleton)) {
        wcout << L"Skeleton 파일 로드 실패: " << skeletonPath << endl;
        return false;
    }

    wofstream ofs(path);
    if (!ofs) {
        wcout << L"베이킹 파일 생성 실패: " << path << endl;
        return false;
    }

    uint32_t boneCount = static_cast<uint32_t>(baseSkeleton.size());
    uint32_t frameCount = 0;

    // 프레임 수 계산
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    if (frameCount == 0) {
        wcout << L"프레임 없음: " << path << endl;
        return true;
    }

    // 헤더 작성
    ofs << L"BAKED_ANIMATION" << endl;
    ofs << L"AnimationName: " << animClip.name << endl;
    ofs << L"BoneCount: " << boneCount << endl;
    ofs << L"FrameCount: " << frameCount << endl;

    float duration = 0.0f;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            duration = static_cast<float>(
                boneFrames.back().time - boneFrames.front().time
                );
            break;
        }
    }
    ofs << L"Duration: " << duration << endl;
    ofs << L"FPS: " << (frameCount / duration) << endl;
    ofs << L"---" << endl;

    // 각 프레임마다 최종 본 행렬 계산
    for (uint32_t frame = 0; frame < frameCount; ++frame) {
        ofs << L"Frame[" << frame << L"]" << endl;

        for (uint32_t boneIdx = 0; boneIdx < boneCount; ++boneIdx) {
            // 1. 애니메이션 변환 가져오기
            FbxAMatrix animTransform;
            if (frame < animClip.keyFrames[boneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[boneIdx][frame];

                FbxVector4 scale = keyFrame.matTransform.GetS();
                FbxQuaternion rotation = keyFrame.matTransform.GetQ();
                FbxVector4 translation = keyFrame.matTransform.GetT();

                FbxAMatrix matScale, matRot;
                matScale.SetS(scale);
                matRot.SetQ(rotation);

                animTransform = matScale * matRot;

                animTransform.mData[3][0] += translation[0];
                animTransform.mData[3][1] += translation[1];
                animTransform.mData[3][2] += translation[2];
            }
            else {
                animTransform.SetIdentity();
            }

            // 2. Offset 행렬과 곱하기 (T-pose 기준)
            // finalTransform = Offset × Animation                              // 아래 두줄 때문에 개고생함 시발 다시는 까먹지 말자
            FbxAMatrix offset = baseSkeleton[boneIdx]->matOffset.Transpose();    
            FbxAMatrix finalTransform = animTransform * offset;                 

            // 내가 왠만해서 이런 주석 안다는데 진짜 벌써 3번째 개고생 한 덕에 단다
            // ---------------------------------------------------------
            // 1. matOffset은 FBXLoader에서 이미 Transpose되어 DirectX Row-major 형식으로 저장됨
            // 2. Transpose()로 다시 FBX Column-major 형식으로 복원
            // 3. FBX SDK의 행렬 곱셈(Column-major)으로 animTransform * offset 계산
            // 4. 이 결과가 DirectX의 mul(offset, animTransform)과 동일한 효과
            // 
            // 핵심: FBX Column-major의 "A * B"는 DirectX Row-major의 "B × A"와 같음
            // 따라서 animTransform * offset = DirectX의 offset × animTransform
            // ---------------------------------------------------------

            // 3. 텍스트로 저장
            ofs << L"  Bone[" << boneIdx << L"]:" << endl;
            for (int row = 0; row < 4; ++row) {
                ofs << L"    ";
                for (int col = 0; col < 4; ++col) {
                    ofs << static_cast<float>(finalTransform.mData[row][col]);
                    if (col < 3) ofs << L" ";
                }
                ofs << endl;
            }
        }
        ofs << endl;
    }

    wcout << L"베이킹 완료: " << path << endl;
    wcout << L"  애니메이션: " << animClip.name << endl;
    wcout << L"  프레임: " << frameCount << L", 본: " << boneCount << endl;
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
