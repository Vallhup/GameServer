#include "pch.h"
#include "Scene.h"
#include "DX12Core.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Material.h"
#include "Camera.h"

void Scene::Initialize(DX12Core& core)
{
    coreRef = &core;

    cam = make_unique<Camera>();
    cam->Initialize();

    Material::InitializeBindlessSystem(coreRef->GetDevice());

    InitializeLogic();

    Material::UpdateMaterialBuffer();

    coreRef->FlushCommandQueue();
    coreRef->ResetCommandQueue();

    Material::ReleaseUploadBuffers();
}

void Scene::Update(const float deltaTime)
{
    UpdateScene(deltaTime);
    cam->Update(*coreRef, deltaTime);
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

void Scene::RenderEffects()
{
    RenderSceneEffects();
}

Camera* Scene::GetCamera() const
{
    return cam.get();
}

void Scene::SetSceneManager(SceneManager* manager)
{
    sManagerRef = manager;
}
