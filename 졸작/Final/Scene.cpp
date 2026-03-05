#include "pch.h"
#include "Scene.h"
#include "SceneManager.h"
#include "MainCharacter.h"
#include "Material.h"
#include "Camera.h"
#include "Input.h"

#include "NetHelper.h"

void Scene::Initialize(HWND hWnd, DX12Core& core)
{
    coreRef = &core;

    if (cam)
        cam.reset();

    cam = make_unique<Camera>();
    cam->Initialize(hWnd);

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

Camera* Scene::GetCamera() const
{
    return cam.get();
}

void Scene::SetSceneManager(SceneManager* manager)
{
    sManagerRef = manager;
}

void Scene::HandlePacket(const PacketHeader & header, const BYTE * data)
{
	PacketType type = static_cast<PacketType>(header.type);

	switch (type) {
	case PacketType::SC_LOGIN:
	{
		return NetHelper::DispatchPacket<Protocol::SC_LOGIN_PACKET>(header, data,
			[this](const auto& packet) { HandleLogin(packet); });
	}
	case PacketType::SC_ADD:
	{
		return NetHelper::DispatchPacket<Protocol::SC_ADD_PACKET>(header, data,
			[this](const auto& packet) { HandleAdd(packet); });
	}
	case PacketType::SC_MOVE_OBJECT:
	{
		return NetHelper::DispatchPacket<Protocol::SC_MOVE_PACKET>(header, data,
			[this](const auto& packet) { HandleMove(packet); });
	}
	case PacketType::SC_REMOVE:
	{
		return NetHelper::DispatchPacket<Protocol::SC_REMOVE_PACKET>(header, data,
			[this](const auto& packet) { HandleRemove(packet); });
	}
	case PacketType::SC_ANIMATION_CHANGE:
	{
		return NetHelper::DispatchPacket<Protocol::SC_ANIMATION_TRANSITION_PACKET>(header, data,
			[this](const auto& packet) { HandleAnimationChange(packet); });
	}
	case PacketType::SC_STAT_CHANGE:
	{
		return NetHelper::DispatchPacket<Protocol::SC_STAT_CHANGE_PACKET>(header, data,
			[this](const auto& packet) { HandleStatChange(packet); });
	}
	}
}