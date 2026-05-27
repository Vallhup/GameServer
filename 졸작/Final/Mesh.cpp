#include "pch.h"
#include "Mesh.h"
#include "GameObject.h"
#include "VertexIndexBuffer.h"
#include "Material.h"
#include "Animator.h"
#include "ResourceManager.h"

void Mesh::SetMesh(DX12Core& core, const wstring& path)
{
    auto startTime = chrono::high_resolution_clock::now();

    auto cachedMesh = RESOURCE.GetCachedMesh(path);
    if (cachedMesh) {
        vertexIndexBuffer = cachedMesh->vertexIndexBuffer;
        cachedRef = cachedMesh;

        materials.clear();   
        material.reset();

        const auto& matIdx = cachedMesh->materialIndices;
        if (matIdx.size() > 1) {
            materials.reserve(matIdx.size());
            for (UINT idx : matIdx)
                materials.push_back(Material::FromExistingIndex(idx));
        }
        else if (matIdx.size() == 1) {
            material = Material::FromExistingIndex(matIdx[0]);
        }

        subMeshes = cachedMesh->subMeshes;
        originalMaterialData = cachedMesh->originalMaterialData;

        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator && cachedMesh->hasAnimation) {
            animator->SetAnimationData(core, cachedMesh->animationClips);
            animator->SetSkeletonData(cachedMesh->skeletonData);
        }

        if (cachedMesh->boundingBox.Extents.x > 0) {
            GetGameObject()->SetLocalBoundingBox(cachedMesh->boundingBox);
        }

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE HIT - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());
        return;
    }

    Importer importer;
    if (importer.LoadModel(path))
    {
        const MeshData& mesh = importer.GetMesh();

        BoundingBox localBox;
        BoundingBox::CreateFromPoints(localBox, mesh.vertices.size(), &mesh.vertices[0].pos, sizeof(Vertex));
        GetGameObject()->SetLocalBoundingBox(localBox);

        vertexIndexBuffer = make_shared<VertexIndexBuffer>();
        vertexIndexBuffer->Initialize(
            core.GetDevice(),
            core.GetActiveCmdList(),
            mesh.vertices,
            mesh.indices
        );

        const auto& mats = importer.GetMaterials();

        // Debug mesh and material info of model
        //DebugMaterialInfo(mesh, mats);

        wstring texBasePath = path.substr(0, path.find_last_of(L"/\\") + 1);

        if (mesh.subMeshes.size() > 1)
        {
            subMeshes = mesh.subMeshes;
            SetMultiMaterials(core, mats, texBasePath);
        }
        else
        {
            SetSingleMaterial(core, mats, texBasePath);
        }

        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator) {
            animator->LoadAnimationFromImporter(core, importer);
        }

        vector<UINT> matIndices;
        if (!materials.empty()) {
            matIndices.reserve(materials.size());
            for (auto& m : materials)
                matIndices.push_back(m->GetMaterialIndex());
        }
        else if (material) {
            matIndices = { material->GetMaterialIndex() };
        }

        RESOURCE.CacheMesh(path, vertexIndexBuffer, matIndices, subMeshes, originalMaterialData,
            mesh.hasAnimation, importer.GetAnimations(), importer.GetSkeleton(), localBox);

        cachedRef = RESOURCE.GetCachedMesh(path);
        StoreCollisionTriangles(mesh);

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE MISS - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());

        OutputDebugStringA("FBX Mesh created for rendering!\n");
    }
    else
        OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}

void Mesh::SetMesh2(DX12Core& core, const wstring& path)
{
    auto startTime = chrono::high_resolution_clock::now();

    auto cachedMesh = RESOURCE.GetCachedMesh(path);
    if (cachedMesh) {
        vertexIndexBuffer = cachedMesh->vertexIndexBuffer;
        cachedRef = cachedMesh;

        materials.clear();   
        material.reset();

        const auto& matIdx = cachedMesh->materialIndices;
        if (matIdx.size() > 1) {
            materials.reserve(matIdx.size());
            for (UINT idx : matIdx)
                materials.push_back(Material::FromExistingIndex(idx));
        }
        else if (matIdx.size() == 1) {
            material = Material::FromExistingIndex(matIdx[0]);
        }

        subMeshes = cachedMesh->subMeshes;
        originalMaterialData = cachedMesh->originalMaterialData;

        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator && cachedMesh->hasAnimation) {
            animator->SetAnimationData(core, cachedMesh->animationClips);
            animator->SetSkeletonData(cachedMesh->skeletonData);
        }

        if (cachedMesh->boundingBox.Extents.x > 0) {
            GetGameObject()->SetLocalBoundingBox(cachedMesh->boundingBox);
        }

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE HIT - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());
        return;
    }

    Importer importer;
    if (importer.LoadModel2(path))
    {
        const MeshData& mesh = importer.GetMesh();

        BoundingBox localBox;
        BoundingBox::CreateFromPoints(localBox, mesh.vertices.size(), &mesh.vertices[0].pos, sizeof(Vertex));
        GetGameObject()->SetLocalBoundingBox(localBox);

        vertexIndexBuffer = make_shared<VertexIndexBuffer>();
        vertexIndexBuffer->Initialize(
            core.GetDevice(),
            core.GetActiveCmdList(),
            mesh.vertices,
            mesh.indices
        );

        const auto& mats = importer.GetMaterials();

        // Debug mesh and material info of model
        //DebugMaterialInfo(mesh, mats);

        wstring texBasePath = path.substr(0, path.find_last_of(L"/\\") + 1);

        if (mesh.subMeshes.size() > 1)
        {
            subMeshes = mesh.subMeshes;
            SetMultiMaterials(core, mats, texBasePath);
        }
        else
        {
            SetSingleMaterial(core, mats, texBasePath);
        }

        auto animator = GetGameObject()->GetComponent<Animator>();
        if (animator) {
            animator->LoadAnimationFromImporter(core, importer);
        }

        vector<UINT> matIndices;
        if (!materials.empty()) {
            matIndices.reserve(materials.size());
            for (auto& m : materials)
                matIndices.push_back(m->GetMaterialIndex());
        }
        else if (material) {
            matIndices = { material->GetMaterialIndex() };
        }

        RESOURCE.CacheMesh(path, vertexIndexBuffer, matIndices, subMeshes, originalMaterialData,
            mesh.hasAnimation, importer.GetAnimations(), importer.GetSkeleton(), localBox);

        cachedRef = RESOURCE.GetCachedMesh(path);
        StoreCollisionTriangles(mesh);

        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        OutputDebugStringA(("CACHE MISS - SetMesh time: " + to_string(duration.count()) + "ms\n").c_str());

        OutputDebugStringA("FBX Mesh created for rendering!\n");
    }
    else
        OutputDebugStringA("Cannot create FBX Mesh for rendering!\n");
}

void Mesh::StoreCollisionTriangles(const MeshData& mesh)
{
    if (!cachedRef || cachedRef->hasAnimation) return;   
    if (!cachedRef->collisionPositions.empty()) return;  

    cachedRef->collisionPositions.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices)
        cachedRef->collisionPositions.push_back(v.pos);
    cachedRef->collisionIndices = mesh.indices;
}

bool Mesh::HasCollisionData() const
{
    return cachedRef && !cachedRef->collisionIndices.empty();
}

const vector<XMFLOAT3>& Mesh::GetCollisionPositions() const
{
    return cachedRef->collisionPositions;
}

const vector<UINT>& Mesh::GetCollisionIndices() const
{
    return cachedRef->collisionIndices;
}

void Mesh::SetCollisionMesh(DX12Core& core, const wstring& path)
{
    Importer importer;
    if (importer.LoadAllCollisionMeshes(path))
    {
        const MeshData& mesh = importer.GetMesh();

        collisionMeshBuffer = make_shared<VertexIndexBuffer>();
        collisionMeshBuffer->Initialize(
            core.GetDevice(),
            core.GetActiveCmdList(),
            mesh.vertices,
            mesh.indices
        );

        OutputDebugStringA(("Collision Mesh loaded - Vertices: " +
            to_string(mesh.vertices.size()) + ", Indices: " +
            to_string(mesh.indices.size()) + "\n").c_str());
    }
    else
    {
        OutputDebugStringA("No collision meshes found\n");
    }
}

void Mesh::ReleaseUploadBuffers()
{
    if (vertexIndexBuffer) {
        vertexIndexBuffer->ReleaseUploadBuffers();
    }
    if (collisionMeshBuffer) {
        collisionMeshBuffer->ReleaseUploadBuffers();
    }
}

void Mesh::SetSingleMaterial(DX12Core& core, const vector<MaterialData>& mats, const wstring& texBasePath)
{
    material = make_shared<Material>();
    material->LoadFromMaterialData(
        core.GetDevice(),
        core.GetActiveCmdList(),
        mats[0],
        texBasePath
    );
}

void Mesh::SetMultiMaterials(DX12Core& core, const vector<MaterialData>& mats, const wstring& texBasePath)
{
    originalMaterialData = mats;

    for (const auto& matData : mats)
    {
        auto mat = make_shared<Material>();
        mat->LoadFromMaterialData(
            core.GetDevice(),
            core.GetActiveCmdList(),
            matData,
            texBasePath
        );

        materials.push_back(mat);
    }
}

void Mesh::DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData>& mats)
{
    OutputDebugStringA(("Total materials found: " + to_string(mats.size()) + "\n").c_str());

    for (size_t i = 0; i < mats.size(); ++i) {
        string msg = "Material[" + to_string(i) + "]: " + mats[i].name + "\n";
        OutputDebugStringA(msg.c_str());

        OutputDebugStringA(("  BaseColor: " + mats[i].baseColorTexPath + "\n").c_str());
        OutputDebugStringA(("  Normal: " + mats[i].normalTexPath + "\n").c_str());
        OutputDebugStringA(("  Roughness: " + mats[i].roughnessTexPath + "\n").c_str());
        OutputDebugStringA(("  Metallic: " + mats[i].metallicTexPath + "\n").c_str());
        OutputDebugStringA(("  Height: " + mats[i].heightTexPath + "\n").c_str());
        OutputDebugStringA(("  Alpha: " + mats[i].alphaTexPath + "\n").c_str());
        OutputDebugStringA(("  Emission: " + mats[i].emissionTexPath + "\n").c_str());
        OutputDebugStringA(("  AO: " + mats[i].aoTexPath + "\n").c_str());
    }

    OutputDebugStringA(("SubMesh count: " + to_string(mesh.subMeshes.size()) + "\n").c_str());

    for (size_t i = 0; i < mesh.subMeshes.size(); ++i) {
        string msg = "SubMesh[" + to_string(i) + "]: ";
        msg += "StartIndex=" + to_string(mesh.subMeshes[i].startIndex) + ", ";
        msg += "IndexCount=" + to_string(mesh.subMeshes[i].indexCount) + ", ";
        msg += "MaterialIndex=" + to_string(mesh.subMeshes[i].materialIndex) + "\n";
        OutputDebugStringA(msg.c_str());
    }
}