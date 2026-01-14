#include "pch.h"
#include "Scene.h"
#include "DX12Core.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Material.h"
#include "Camera.h"
#include "Input.h"
#include "SceneRenderer.h"
#include "GameObject.h"
#include "Transform.h"

void Scene::Initialize(HWND hWnd, DX12Core& core)
{
    coreRef = &core;

    if (cam)
        cam.reset();

    cam = make_unique<Camera>();
    cam->Initialize(hWnd);

    InitializeObjectPools();

    InitializeLogic();

    coreRef->FlushCommandQueue();
    coreRef->ResetCommandQueue();

    Material::ReleaseUploadBuffers();
}

void Scene::Update(const float deltaTime)
{
    UpdateScene(deltaTime);
    
    // Temporarily test in GameScene Only
    //if (cam)
    //    cam->Update(*coreRef, deltaTime, );
    
    RequestSceneChange();
}

void Scene::RenderDeferred()
{
    RenderSceneDeferred();
}

void Scene::RenderForward()
{
    RenderSceneForward();
}

void Scene::RenderShadow()
{
    RenderSceneShadow();
}

void Scene::RenderEffects()
{
    RenderSceneEffects();
}

void Scene::InitializeObjectPools()
{
    InitializeSceneObjectPools();
}

void Scene::InitializeInstanceGroup(InstanceGroup& group)
{
    if (group.objects.empty()) return;

    size_t bufferSize = sizeof(XMMATRIX) * group.objects.size();
    group.instanceBuffer = make_unique<UploadBuffer>();
    group.instanceBuffer->Initialize(coreRef->GetDevice(), bufferSize);

    group.fullInstanceBuffer = make_unique<UploadBuffer>();
    group.fullInstanceBuffer->Initialize(coreRef->GetDevice(), bufferSize);

    vector<XMMATRIX> transforms;
    transforms.reserve(group.objects.size());
    for (const auto& obj : group.objects) {
        auto transform = obj->GetComponent<Transform>();
        transforms.push_back(XMMatrixTranspose(transform->GetWorldMatrix()));
    }

    group.instanceBuffer->CopyData(transforms.data(), bufferSize, 0);
    group.fullInstanceBuffer->CopyData(transforms.data(), bufferSize, 0);
}

void Scene::UpdateInstanceGroup(InstanceGroup& group, const BoundingFrustum& frustum)
{
    vector<XMMATRIX> visibleTransforms;
    visibleTransforms.reserve(group.objects.size());

    for (const auto& obj : group.objects) {
        if (obj->IsInFrustum(frustum)) {
            auto transform = obj->GetComponent<Transform>();
            visibleTransforms.push_back(XMMatrixTranspose(transform->GetWorldMatrix()));
        }
    }

    group.visibleCount = visibleTransforms.size();
    if (group.visibleCount > 0) {
        group.instanceBuffer->CopyData(visibleTransforms.data(), sizeof(XMMATRIX) * group.visibleCount, 0);
    }
}

Camera* Scene::GetCamera() const
{
    return cam.get();
}

void Scene::SetSceneManager(SceneManager* manager)
{
    sManagerRef = manager;
}
