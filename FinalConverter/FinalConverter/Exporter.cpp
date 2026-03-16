#include "pch.h"
#include "Exporter.h"

bool Exporter::ExportAll(FBXLoader& loader, const wstring& basePath, const wstring& fbxDir,
    const wstring& referenceSkeletonPath)
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

    // Load reference skeleton if specified
    vector<shared_ptr<FbxBoneInfo>> refBones;
    bool hasRefSkeleton = false;
    if (!referenceSkeletonPath.empty()) {
        if (LoadSkeletonFromFile(referenceSkeletonPath, refBones)) {
            hasRefSkeleton = true;
            wcout << L"Reference skeleton loaded: " << referenceSkeletonPath << endl;
            wcout << L"  Ref bones: " << refBones.size() << L", Anim bones: " << loader.GetBones().size() << endl;
        }
        else {
            wcout << L"Failed to load reference skeleton: " << referenceSkeletonPath << endl;
        }
    }

    for (auto& animClip : loader.GetAnimClip()) {
        wstring animName = animClip->name;
        replace(animName.begin(), animName.end(), L'|', L'_');
        wstring animPath = basePath + L"_" + animName + L".anim";
        wstring animTextPath = basePath + L"_Text_" + animName + L".anim";
        wstring bakedPath = basePath + L"_" + animName + L"_baked.bone";

        if (hasRefSkeleton) {
            // Use reference skeleton mapping
            if (!ExportAnimationWithMapping(*animClip, loader.GetBones(), refBones, animPath))
                return false;
            if (!ExportAnimationAsTextWithMapping(*animClip, loader.GetBones(), refBones, animTextPath))
                return false;
            if (!ExportBakedAnimation(loader.GetBones(), refBones, *animClip, bakedPath))
                return false;
        }
        else {
            // Original method (no mapping)
            if (!ExportAnimation(*animClip, animPath)) return false;
            if (!ExportAnimationAsText(*animClip, animTextPath)) return false;
            // ExportBakedAnimation requires reference skeleton - skip
        }
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
        wcout << L"Failed to create mesh file: " << path << endl;
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

    wcout << L"Mesh export complete: " << path << endl;
    wcout << L"  Vertices: " << header.vertexCount << L", Indices: " << header.indexCount
        << L", SubMeshes: " << header.subMeshCount << endl;
    return true;
}

bool Exporter::ExportMeshAsText(const FbxMeshInfo& meshInfo, const wstring& path)
{
    wofstream ofs(path);
    if (!ofs) {
        wcout << L"Failed to create mesh text file: " << path << endl;
        return false;
    }

    // Header info
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

    // Vertex data
    ofs << L"\n[VERTICES]" << endl;
    for (size_t i = 0; i < meshInfo.vertices.size(); ++i) {
        const auto& v = meshInfo.vertices[i];

        ofs << L"Vertex[" << i << L"]" << endl;
        ofs << L"  Position: " << v.pos.x << L" " << v.pos.y << L" " << v.pos.z << endl;
        ofs << L"  Normal: " << v.normal.x << L" " << v.normal.y << L" " << v.normal.z << endl;
        ofs << L"  UV: " << v.uv.x << L" " << v.uv.y << endl;
        ofs << L"  Tangent: " << v.tangent.x << L" " << v.tangent.y << L" " << v.tangent.z << endl;

        // Animation weights (if present)
        if (meshInfo.hasAnimation) {
            ofs << L"  BoneIndices: " << v.indices.x << L" " << v.indices.y << L" "
                << v.indices.z << L" " << v.indices.w << endl;
            ofs << L"  BoneWeights: " << v.weights.x << L" " << v.weights.y << L" "
                << v.weights.z << L" " << v.weights.w << endl;
        }
    }

    // SubMesh info
    ofs << L"\n[SUBMESHES]" << endl;
    uint32_t currentOffset = 0;
    for (size_t i = 0; i < meshInfo.indices.size(); ++i) {
        ofs << L"SubMesh[" << i << L"]" << endl;
        ofs << L"  StartIndex: " << currentOffset << endl;
        ofs << L"  IndexCount: " << meshInfo.indices[i].size() << endl;
        ofs << L"  MaterialIndex: " << i << endl;

        currentOffset += static_cast<uint32_t>(meshInfo.indices[i].size());
    }

    // Index data
    ofs << L"\n[INDICES]" << endl;
    for (size_t subMeshIdx = 0; subMeshIdx < meshInfo.indices.size(); ++subMeshIdx) {
        ofs << L"SubMesh[" << subMeshIdx << L"] Indices:" << endl;

        const auto& indices = meshInfo.indices[subMeshIdx];

        // Output per triangle (for readability)
        for (size_t i = 0; i < indices.size(); i += 3) {
            ofs << L"  Triangle[" << (i / 3) << L"]: ";
            ofs << indices[i] << L" " << indices[i + 1] << L" " << indices[i + 2] << endl;
        }
    }

    wcout << L"Mesh text export complete: " << path << endl;
    wcout << L"  Vertices: " << meshInfo.vertices.size() << L", Indices: " << totalIndices
        << L", SubMeshes: " << meshInfo.indices.size() << endl;

    return true;
}

bool Exporter::ExportSkeleton(const vector<shared_ptr<FbxBoneInfo>>& bones, const wstring& path)
{
    if (bones.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"Failed to create skeleton file: " << path << endl;
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

    wcout << L"Skeleton export complete: " << path << endl;
    wcout << L"  Bone count: " << header.boneCount << endl;
    return true;
}

bool Exporter::ExportSkeletonText(const vector<shared_ptr<FbxBoneInfo>>& bones, const wstring& path)
{
    if (bones.empty()) return true;

    wofstream ofs(path);
    if (!ofs) {
        wcout << L"Failed to create skeleton text file: " << path << endl;
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

// ExportAnimation function - FBX Quaternion type issue resolved
bool Exporter::ExportAnimation(const FbxAnimClipInfo& animClip, const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"Failed to create animation file: " << path << endl;
        return false;
    }

    AnimationBinaryHeader header = {};
    header.magic = 'MINA';
    header.boneCount = static_cast<uint32_t>(animClip.keyFrames.size());

    // Get frameCount from the first bone that has keyframes
    header.frameCount = 0;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            header.frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    // Calculate duration
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

    // Save in fast update layout: [frame * boneCount + bone]
    for (uint32_t frameIdx = 0; frameIdx < header.frameCount; ++frameIdx) {
        for (uint32_t boneIdx = 0; boneIdx < header.boneCount; ++boneIdx) {

            if (frameIdx < animClip.keyFrames[boneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[boneIdx][frameIdx];

                // Extract SQT
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
                // Default values if no frame
                struct AnimFrameParams {
                    float scale[4] = { 1,1,1,1 };
                    float rotation[4] = { 0,0,0,1 };
                    float translation[4] = { 0,0,0,0 };
                } params;

                ofs.write(reinterpret_cast<const char*>(&params), sizeof(params));
            }
        }
    }

    wcout << L"Animation export complete: " << path << endl;
    return true;
}

bool Exporter::ExportAnimationAsText(const FbxAnimClipInfo& animClip, const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path);
    if (!ofs) {
        wcout << L"Failed to create animation text file: " << path << endl;
        return false;
    }

    AnimationBinaryHeader header = {};
    header.magic = 'MINA';
    header.boneCount = static_cast<uint32_t>(animClip.keyFrames.size());

    // Get frameCount from the first bone that has keyframes
    header.frameCount = 0;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            header.frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    // Calculate duration
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

    wcout << L"Animation text export complete: " << path << endl;
    return true;
}

bool Exporter::ExportMaterials(const vector<FbxMaterialInfo>& materials, const wstring& path)
{
    if (materials.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"Failed to create material file: " << path << endl;
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

    wcout << L"Material export complete: " << path << endl;
    wcout << L"  Material count: " << header.materialCount << endl;
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

    wcout << L"Texture copy complete: " << copiedCount << L" files copied" << endl;
    return true;
}

bool Exporter::LoadSkeletonFromFile(const wstring& path, vector<shared_ptr<FbxBoneInfo>>& bones)
{
    ifstream ifs(path, ios::binary);
    if (!ifs) {
        wcout << L"Failed to open skeleton file: " << path << endl;
        return false;
    }

    SkeletonBinaryHeader header = {};
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 'LEKS') {
        wcout << L"Invalid skeleton file magic: " << path << endl;
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

    wcout << L"Skeleton loaded: " << bones.size() << L" bones" << endl;
    return !bones.empty();
}

bool Exporter::ExportBakedAnimation(
    const vector<shared_ptr<FbxBoneInfo>>& animBones,
    const vector<shared_ptr<FbxBoneInfo>>& refBones,
    const FbxAnimClipInfo& animClip,
    const wstring& path)
{
    if (animClip.keyFrames.empty()) {
        wcout << L"No frames to bake: " << path << endl;
        return true;
    }

    wofstream ofs(path);
    if (!ofs) {
        wcout << L"Failed to create baked file: " << path << endl;
        return false;
    }

    uint32_t refBoneCount = static_cast<uint32_t>(refBones.size());
    uint32_t frameCount = 0;

    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    if (frameCount == 0) {
        wcout << L"No frames: " << path << endl;
        return true;
    }

    // Bone mapping table: refBoneIdx -> animBoneIdx
    vector<int32_t> boneMapping(refBoneCount, -1);
    int mappedCount = 0;
    for (uint32_t refIdx = 0; refIdx < refBoneCount; ++refIdx) {
        boneMapping[refIdx] = FindAnimBoneIndex(animBones, refBones[refIdx]->boneName);
        if (boneMapping[refIdx] >= 0) mappedCount++;
    }

    // Write header
    ofs << L"BAKED_ANIMATION" << endl;
    ofs << L"AnimationName: " << animClip.name << endl;
    ofs << L"BoneCount: " << refBoneCount << endl;
    ofs << L"FrameCount: " << frameCount << endl;

    float duration = 0.0f;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            duration = static_cast<float>(boneFrames.back().time - boneFrames.front().time);
            break;
        }
    }
    ofs << L"Duration: " << duration << endl;
    ofs << L"FPS: " << (frameCount / duration) << endl;
    ofs << L"BoneMapped: " << mappedCount << L"/" << refBoneCount << endl;
    ofs << L"---" << endl;

    // Output final bone matrices for each frame
    for (uint32_t frame = 0; frame < frameCount; ++frame) {
        ofs << L"Frame[" << frame << L"]" << endl;

        for (uint32_t refBoneIdx = 0; refBoneIdx < refBoneCount; ++refBoneIdx) {
            int32_t animBoneIdx = boneMapping[refBoneIdx];

            // 1. Get animation transform
            FbxAMatrix animTransform;
            if (animBoneIdx >= 0 && frame < animClip.keyFrames[animBoneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[animBoneIdx][frame];

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

            // 2. Multiply with reference skeleton's offset matrix
            FbxAMatrix offset = refBones[refBoneIdx]->matOffset.Transpose();
            FbxAMatrix finalTransform = animTransform * offset;

            // 3. Save as text
            ofs << L"  Bone[" << refBoneIdx << L"] (" << refBones[refBoneIdx]->boneName << L"):" << endl;
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

    wcout << L"Baking complete: " << path << endl;
    wcout << L"  Animation: " << animClip.name << endl;
    wcout << L"  Frames: " << frameCount << L", Bones: " << refBoneCount << endl;
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
    // Reflection matrix for Mixamo animation (Y-Z axis swap)
    FbxVector4 v1 = { 1, 0, 0, 0 };
    FbxVector4 v2 = { 0, 0, 1, 0 };  // Y-Z swap
    FbxVector4 v3 = { 0, 1, 0, 0 };
    FbxVector4 v4 = { 0, 0, 0, 1 };

    FbxAMatrix matReflect;
    matReflect.mData[0] = v1;
    matReflect.mData[1] = v2;
    matReflect.mData[2] = v3;
    matReflect.mData[3] = v4;

    // reflection * matrix * reflection form (Mixamo method)
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
        wcout << L"Texture not found: " << sourcePath << endl;
        return false;
    }

    if (CopyFileW(sourcePath.c_str(), destPath.c_str(), FALSE)) {
        wcout << L"Texture copied: " << GetRelativeTexturePath(sourcePath) << endl;
        return true;
    }
    else {
        wcout << L"Texture copy failed: " << sourcePath << endl;
        return false;
    }
}

int32_t Exporter::FindAnimBoneIndex(const vector<shared_ptr<FbxBoneInfo>>& animBones, const wstring& boneName)
{
    for (size_t i = 0; i < animBones.size(); ++i) {
        if (animBones[i]->boneName == boneName) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

bool Exporter::ExportAnimationWithMapping(
    const FbxAnimClipInfo& animClip,
    const vector<shared_ptr<FbxBoneInfo>>& animBones,
    const vector<shared_ptr<FbxBoneInfo>>& refBones,
    const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path, ios::binary);
    if (!ofs) {
        wcout << L"Failed to create animation file: " << path << endl;
        return false;
    }

    // Use reference skeleton bone count
    uint32_t refBoneCount = static_cast<uint32_t>(refBones.size());

    AnimationBinaryHeader header = {};
    header.magic = 'MINA';
    header.boneCount = refBoneCount;

    // Calculate frame count
    header.frameCount = 0;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            header.frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

    // Calculate duration
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

    // Create bone name mapping table: refBoneIdx -> animBoneIdx
    vector<int32_t> boneMapping(refBoneCount, -1);
    int mappedCount = 0;
    for (uint32_t refIdx = 0; refIdx < refBoneCount; ++refIdx) {
        int32_t animIdx = FindAnimBoneIndex(animBones, refBones[refIdx]->boneName);
        boneMapping[refIdx] = animIdx;
        if (animIdx >= 0) mappedCount++;
    }
    wcout << L"  Bone mapping: " << mappedCount << L"/" << refBoneCount << L" matched" << endl;

    // Save keyframes in reference skeleton order
    for (uint32_t frameIdx = 0; frameIdx < header.frameCount; ++frameIdx) {
        for (uint32_t refBoneIdx = 0; refBoneIdx < refBoneCount; ++refBoneIdx) {
            int32_t animBoneIdx = boneMapping[refBoneIdx];

            struct AnimFrameParams {
                float scale[4];
                float rotation[4];
                float translation[4];
            } params;

            if (animBoneIdx >= 0 && frameIdx < animClip.keyFrames[animBoneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[animBoneIdx][frameIdx];

                FbxVector4 scale = keyFrame.matTransform.GetS();
                FbxQuaternion rotation = keyFrame.matTransform.GetQ();
                FbxVector4 translation = keyFrame.matTransform.GetT();

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
            }
            else {
                // Default values for unmatched bones
                params.scale[0] = 1; params.scale[1] = 1; params.scale[2] = 1; params.scale[3] = 1;
                params.rotation[0] = 0; params.rotation[1] = 0; params.rotation[2] = 0; params.rotation[3] = 1;
                params.translation[0] = 0; params.translation[1] = 0; params.translation[2] = 0; params.translation[3] = 0;
            }

            ofs.write(reinterpret_cast<const char*>(&params), sizeof(params));
        }
    }

    wcout << L"Animation saved (with mapping): " << path << endl;
    return true;
}

bool Exporter::ExportAnimationAsTextWithMapping(
    const FbxAnimClipInfo& animClip,
    const vector<shared_ptr<FbxBoneInfo>>& animBones,
    const vector<shared_ptr<FbxBoneInfo>>& refBones,
    const wstring& path)
{
    if (animClip.keyFrames.empty()) return true;

    ofstream ofs(path);
    if (!ofs) {
        wcout << L"Failed to create animation file: " << path << endl;
        return false;
    }

    uint32_t refBoneCount = static_cast<uint32_t>(refBones.size());

    AnimationBinaryHeader header = {};
    header.magic = 'MINA';
    header.boneCount = refBoneCount;

    header.frameCount = 0;
    for (const auto& boneFrames : animClip.keyFrames) {
        if (!boneFrames.empty()) {
            header.frameCount = static_cast<uint32_t>(boneFrames.size());
            break;
        }
    }

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

    // Bone mapping table
    vector<int32_t> boneMapping(refBoneCount, -1);
    for (uint32_t refIdx = 0; refIdx < refBoneCount; ++refIdx) {
        boneMapping[refIdx] = FindAnimBoneIndex(animBones, refBones[refIdx]->boneName);
    }

    for (uint32_t frameIdx = 0; frameIdx < header.frameCount; ++frameIdx) {
        for (uint32_t refBoneIdx = 0; refBoneIdx < refBoneCount; ++refBoneIdx) {
            int32_t animBoneIdx = boneMapping[refBoneIdx];

            ofs << "Frame[" << frameIdx << "] Bone[" << refBoneIdx << "] ("
                << ws2s(refBones[refBoneIdx]->boneName) << ")" << endl;

            if (animBoneIdx >= 0 && frameIdx < animClip.keyFrames[animBoneIdx].size()) {
                const auto& keyFrame = animClip.keyFrames[animBoneIdx][frameIdx];

                FbxVector4 scale = keyFrame.matTransform.GetS();
                FbxQuaternion rotation = keyFrame.matTransform.GetQ();
                FbxVector4 translation = keyFrame.matTransform.GetT();

                ofs << "  scale: " << static_cast<float>(scale[0]) << " "
                    << static_cast<float>(scale[1]) << " "
                    << static_cast<float>(scale[2]) << " 1" << endl;

                ofs << "  rotation: " << static_cast<float>(rotation[0]) << " "
                    << static_cast<float>(rotation[1]) << " "
                    << static_cast<float>(rotation[2]) << " "
                    << static_cast<float>(rotation[3]) << endl;

                ofs << "  translation: " << static_cast<float>(translation[0]) << " "
                    << static_cast<float>(translation[1]) << " "
                    << static_cast<float>(translation[2]) << " 0" << endl;
            }
            else {
                ofs << "  scale: 1 1 1 1" << endl;
                ofs << "  rotation: 0 0 0 1" << endl;
                ofs << "  translation: 0 0 0 0" << endl;
            }
        }
    }

    wcout << L"Animation text saved (with mapping): " << path << endl;
    return true;
}
