#include "pch.h"
#include "InstancingBatch.h"
#include "Mesh.h"
#include "GameObject.h"
#include "Transform.h"
#include "SceneRenderer.h"
#include "Material.h"

void InstancingBatch::Initialize(Mesh* m)
{
	mesh = m;
}

void InstancingBatch::AddObject(shared_ptr<GameObject> obj)
{
    objects.push_back(obj);
}

void InstancingBatch::BuildBuffers(DX12Core& core)
{
    if (objects.empty()) return;

    size_t count = objects.size();
    size_t bufferSize = sizeof(XMMATRIX) * count;

    instanceBuffer = make_unique<UploadBuffer>();
    instanceBuffer->Initialize(core.GetDevice(), bufferSize);

    fullInstanceBuffer = make_unique<UploadBuffer>();
    fullInstanceBuffer->Initialize(core.GetDevice(), bufferSize);

    visibleTransforms.reserve(count);
    shadowTransforms.reserve(count);
    cachedData.reserve(count);

    vector<XMMATRIX> transforms;
    transforms.reserve(count);

    for (const auto& obj : objects) {
        auto transform = obj->GetComponent<Transform>();
        XMMATRIX worldMat = XMMatrixTranspose(transform->GetWorldMatrix());
        transforms.push_back(worldMat);

        CachedInstanceData data;
        data.worldMatrix = worldMat;
        data.position = transform->GetPosition();
        data.boundingBox = obj->GetWorldBoundingBox();
        data.cullDistance = obj->GetCullDistance();
        data.needDistanceCull = obj->NeedDistanceCull();
        cachedData.push_back(data);
    }

    instanceBuffer->CopyData(transforms.data(), bufferSize, 0);
    fullInstanceBuffer->CopyData(transforms.data(), bufferSize, 0);

    if (mesh->HasMultiMaterial())
    {
        const auto& materials = mesh->GetMaterials();
        objectCBs.resize(materials.size());

        for (size_t i = 0; i < materials.size(); ++i)
        {
            objectCBs[i] = make_unique<UploadBuffer>();
            objectCBs[i]->Initialize(core.GetDevice(), CONSTANT_BUFFER_ALIGNMENT);

            UINT matIndex = materials[i]->GetMaterialIndex();
            ObjectConstants obj = {};
            obj.world = XMMatrixTranspose(XMMatrixIdentity());
            obj.useTexture = 1;
            obj.useInstancing = 1;
            obj.materialIndex = matIndex;
            objectCBs[i]->CopyData(&obj, sizeof(ObjectConstants), 0);
        }
    }
    else
    {
        objectCBs.resize(1);
        objectCBs[0] = make_unique<UploadBuffer>();
        objectCBs[0]->Initialize(core.GetDevice(), CONSTANT_BUFFER_ALIGNMENT);

        UINT matIndex = mesh->GetMaterial() ? mesh->GetMaterial()->GetMaterialIndex() : 0;
        ObjectConstants obj = {};
        obj.world = XMMatrixTranspose(XMMatrixIdentity());
        obj.useTexture = 1;
        obj.useInstancing = 1;
        obj.materialIndex = matIndex;
        objectCBs[0]->CopyData(&obj, sizeof(ObjectConstants), 0);
    }
}

void InstancingBatch::Update(const BoundingFrustum& frustum, const XMVECTOR& camPos)
{
    if (cachedData.empty()) return;

    // 카메라 이동 체크 - 충분히 이동하지 않았으면 스킵
    XMFLOAT3 currentCamPos;
    XMStoreFloat3(&currentCamPos, camPos);

    float dx = currentCamPos.x - lastCamPos.x;
    float dy = currentCamPos.y - lastCamPos.y;
    float dz = currentCamPos.z - lastCamPos.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq < UPDATE_THRESHOLD_SQ && visibleCount > 0)
        return;

    lastCamPos = currentCamPos;

    visibleTransforms.clear();
    shadowTransforms.clear();

    float camX = currentCamPos.x;
    float camZ = currentCamPos.z;
    constexpr float shadowRange = 50.0f;

    for (const auto& data : cachedData) {
        // Frustum culling (캐싱된 bounding box 사용)
        bool isVisible = true;
        if (data.boundingBox.Extents.x > 0.0f) {
            isVisible = frustum.Intersects(data.boundingBox);
        }

        // Distance culling
        if (isVisible && data.needDistanceCull) {
            float objDistSq = (data.position.x - camX) * (data.position.x - camX) +
                              (data.position.z - camZ) * (data.position.z - camZ);
            if (objDistSq > data.cullDistance * data.cullDistance)
                isVisible = false;
        }

        // Shadow range 체크
        bool isNearPlayer = (abs(data.position.x - camX) <= shadowRange) &&
                            (abs(data.position.z - camZ) <= shadowRange);

        if (isVisible) {
            visibleTransforms.push_back(data.worldMatrix);
        }

        if (isNearPlayer) {
            shadowTransforms.push_back(data.worldMatrix);
        }
    }

    visibleCount = static_cast<UINT>(visibleTransforms.size());
    shadowCount = static_cast<UINT>(shadowTransforms.size());

    if (visibleCount > 0) {
        instanceBuffer->CopyData(visibleTransforms.data(), sizeof(XMMATRIX) * visibleCount, 0);
    }

    if (shadowCount > 0) {
        fullInstanceBuffer->CopyData(shadowTransforms.data(), sizeof(XMMATRIX) * shadowCount, 0);
    }
}

void InstancingBatch::Render(DX12Core& core, SceneRenderer* renderer)
{
    if (visibleCount == 0 || !mesh || !instanceBuffer) return;

    renderer->RenderInstanced(core, mesh, visibleCount, instanceBuffer.get(), this);
}

void InstancingBatch::RenderShadow(DX12Core& core, SceneRenderer* renderer)
{
    if (!castShadow) return;
    if (shadowCount == 0 || !mesh || !fullInstanceBuffer) return;

    //renderer->RenderInstancedShadow(core, mesh, static_cast<UINT>(objects.size()), fullInstanceBuffer.get());
    renderer->RenderInstancedShadow(core, mesh, shadowCount, fullInstanceBuffer.get(), this);
}

void InstancingBatch::Clear()
{
    objects.clear();
    cachedData.clear();
    cachedData.shrink_to_fit();
    instanceBuffer.reset();
    fullInstanceBuffer.reset();
    visibleTransforms.clear();
    visibleTransforms.shrink_to_fit();
    shadowTransforms.clear();
    shadowTransforms.shrink_to_fit();
    visibleCount = 0;
    shadowCount = 0;
    lastCamPos = { FLT_MAX, FLT_MAX, FLT_MAX };
    objectCBs.clear();
}