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
#include "Engine.h"

int myId{ -1 };

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

void ServerTestScene::HandlePacket(const Protocol::GamePacket& packet)
{
    const auto& header = packet.header();

    switch (header.type()) {
    case Protocol::PacketType::SC_LOGIN: {
        OutputDebugStringA("SC_LOGIN packet received\n");
        Protocol::SC_LOGIN_PACKET login;
        if (login.ParseFromArray(packet.body().data(), packet.body().size())) {
            myId = packet.header().sessionid();
            OutputDebugStringA(("My Session ID: " + to_string(myId) + "\n").c_str());
        }
        break;
    }
    case Protocol::PacketType::SC_ADD: {
        OutputDebugStringA("SC_ADD packet received\n");
        Protocol::SC_ADD_PACKET add;
        if (add.ParseFromArray(packet.body().data(), packet.body().size())) {
            int sessionId = packet.header().sessionid();
            Protocol::Vec3 pos = add.pos();

            // 내 캐릭터만 처리
            if (sessionId == myId && knight) {
                if (auto transform = knight->GetComponent<Transform>()) {
                    transform->SetPosition(pos.x(), pos.y(), pos.z());
                }

                knight->SetCamera(cam.get());

                OutputDebugStringA("My character positioned!\n");
            }

            else if(sessionId != myId and otherKnight) {
                if (auto transform = otherKnight->GetComponent<Transform>()) {
                    transform->SetPosition(pos.x(), pos.y(), pos.z());
                }

                OutputDebugStringA("Other Character positioned!\n");
            }
        }
        break;
    }
    case Protocol::PacketType::SC_MOVE_OBJECT: {
        Protocol::SC_MOVE_PACKET move;
        if (move.ParseFromArray(packet.body().data(), packet.body().size())) {
            int sessionId = packet.header().sessionid();
            Protocol::Vec3 pos = move.pos();

            if (sessionId == myId && knight) {
                if (auto transform = knight->GetComponent<Transform>()) {
                    transform->SetPosition(pos.x(), pos.y(), pos.z());
                    transform->SetTargetRotation(move.rot());
                }
            }

            else if (sessionId != myId and otherKnight) {
                if (auto transform = otherKnight->GetComponent<Transform>()) {
                    transform->SetPosition(pos.x(), pos.y(), pos.z());
                    transform->SetTargetRotation(move.rot());
                }
            }
        }
        break;
    }
    case Protocol::PacketType::SC_REMOVE: {
        OutputDebugStringA("SC_REMOVE packet received\n");
        break;
    }
    }
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
        transform->SetInitPosition(1.f, 0.f, 0.5f);
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
        transform->SetRotation(-1.57f, 0.f, 0.f);
        transform->SetScale(0.01f, 0.01f, 0.01f);
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

    SetNetworkManager(GET(Engine).GetNetworkManager());
    _nManager->Send(PacketFactory::CSLoginPacket());
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