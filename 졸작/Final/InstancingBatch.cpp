#include "pch.h"
#include "InstancingBatch.h"
#include "Mesh.h"
#include "GameObject.h"
#include "Transform.h"
#include "SceneRenderer.h"

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

    vector<XMMATRIX> transforms;
    transforms.reserve(count);
    for (const auto& obj : objects) {
        auto transform = obj->GetComponent<Transform>();
        transforms.push_back(XMMatrixTranspose(transform->GetWorldMatrix()));
    }

    instanceBuffer->CopyData(transforms.data(), bufferSize, 0);
    fullInstanceBuffer->CopyData(transforms.data(), bufferSize, 0);
}

void InstancingBatch::Update(const BoundingFrustum& frustum, const XMVECTOR& camPos)
{
    if (objects.empty()) return;

    vector<XMMATRIX> visibleTransforms;
    visibleTransforms.reserve(objects.size());

    for (const auto& obj : objects) {
        if (obj->IsVisible(frustum, camPos)) {
            visibleTransforms.push_back(XMMatrixTranspose(obj->GetComponent<Transform>()->GetWorldMatrix()));
        }
    }

    visibleCount = static_cast<UINT>(visibleTransforms.size());

    if (visibleCount > 0) {
        instanceBuffer->CopyData(visibleTransforms.data(), sizeof(XMMATRIX) * visibleCount, 0);
    }
}

void InstancingBatch::Render(DX12Core& core, SceneRenderer* renderer)
{
    if (visibleCount == 0 || !mesh || !instanceBuffer) return;

    renderer->RenderInstanced(core, mesh, visibleCount, instanceBuffer.get());
}

void InstancingBatch::RenderShadow(DX12Core& core, SceneRenderer* renderer)
{
    if (!castShadow) return;
    if (objects.empty() || !mesh || !fullInstanceBuffer) return;

    renderer->RenderInstancedShadow(core, mesh, static_cast<UINT>(objects.size()), fullInstanceBuffer.get());
}

void InstancingBatch::Clear()
{
    objects.clear();
    instanceBuffer.reset();
    fullInstanceBuffer.reset();
    visibleCount = 0;
}