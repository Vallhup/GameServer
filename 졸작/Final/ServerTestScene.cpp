#include "pch.h"
#include "ServerTestScene.h"
#include "Material.h"
#include "GameObject.h"
#include "DX12Core.h"
#include "MeshRenderer.h"
#include "SceneManager.h"
#include "Input.h"
#include "Camera.h"
#include "Transform.h"
#include "Animator.h"
#include "MainCharacter.h"

void ServerTestScene::Release()
{
}

void ServerTestScene::Reset()
{
    knight.reset();
    gameObjects.clear();
    Material::Cleanup();
    OutputDebugStringA("ServerTestScene Data has been deleted!! \n----------------------------------------\n");
}

void ServerTestScene::AddGameObject(shared_ptr<GameObject> obj)
{
    gameObjects.push_back(obj);
}

const float* ServerTestScene::GetBackgroundColor()
{
    return Colors::Aqua;
}

void ServerTestScene::InitializeLogic()
{
    OutputDebugStringA("----------------------------------------\nServerTestScene Data has been created!! \n");

    {
        knight = make_shared<MainCharacter>();
        auto meshRenderer = knight->AddComponent<MeshRenderer>();
        auto transform = knight->AddComponent<Transform>();
        auto animator = knight->AddComponent<Animator>();

        meshRenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");
        transform->SetInitPosition(2.f, 0.f, 0.5f);
        transform->SetRotation(-1.57f, 0.f, 0.f);
        transform->SetScale(0.01f, 0.01f, 0.01f);
        knight->SetCamera(cam.get());
        AddGameObject(knight);

        OutputDebugStringA("Knight created!!\n");
    }

    {
        auto dragon = make_shared<GameObject>();
        auto meshRenderer = dragon->AddComponent<MeshRenderer>();
        auto transform = dragon->AddComponent<Transform>();
        auto animator = dragon->AddComponent<Animator>();

        meshRenderer->SetMesh(*coreRef, L"../FBXOutput/Dragon");
        transform->SetInitPosition(0.f, 0.f, 0.5f);
        transform->SetRotation(0.f, 0.f, 0.f);
        transform->SetScale(0.1f, 0.1f, 0.1f);
        AddGameObject(dragon); 

        OutputDebugStringA("Dragon created!!\n");
    }

    OutputDebugStringA("Before FlushCommandQueue - uploadBuffers exist\n");
    coreRef->FlushCommandQueue();
    coreRef->ResetCommandQueue();

    for (const auto& obj : gameObjects) {
        if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
            meshRenderer->ReleaseUploadBuffers();
    }   
    OutputDebugStringA("After ReleaseUploadBuffers - uploadBuffers released\n");
}

void ServerTestScene::UpdateScene(const float deltaTime)
{
    if (coreRef == nullptr) return;

    for (const auto& obj : gameObjects)
        obj->Update(deltaTime);
}

void ServerTestScene::RenderSceneDeferred()
{
    for (const auto& obj : gameObjects) {
        if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
            meshRenderer->RenderDeferred(*coreRef);
    }
}

void ServerTestScene::RenderSceneForward()
{
    for (const auto& obj : gameObjects) {
        if (auto meshRenderer = obj->GetComponent<MeshRenderer>())
            meshRenderer->RenderForward(*coreRef);
    }
}

void ServerTestScene::RenderSceneEffects()
{
}

int ServerTestScene::GetSceneWidth() const
{
    return 0;
}

void ServerTestScene::RequestSceneChange()
{
  
}