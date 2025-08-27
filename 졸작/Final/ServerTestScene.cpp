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
	_objects.clear();

	Material::Cleanup();
}

void ServerTestScene::AddGameObject(const shared_ptr<GameObject>& obj)
{
	_objects.try_emplace(obj->GetId(), obj);
}

void ServerTestScene::HandlePacket(const Protocol::GamePacket& packet)
{
	static int myId{ 0 };
	const auto& header = packet.header();

	switch (header.type()) {
	case Protocol::PacketType::SC_LOGIN:{
		Protocol::SC_LOGIN_PACKET login;
		if (login.ParseFromArray(packet.body().data(), packet.body().size())) {
			myId = packet.header().sessionid();
		}
		break;
	}
	case Protocol::PacketType::SC_ADD: {
		OutputDebugStringA("SC_ADD apcket received\n");

		Protocol::SC_ADD_PACKET add;
		if (add.ParseFromArray(packet.body().data(), packet.body().size())) {
			shared_ptr<GameObject> object;
			if (packet.header().sessionid() == myId) {
				auto mainObj = make_shared<MainCharacter>();
				object = mainObj;

				mainObj->SetCamera(cam.get());
			}

			else {
				object = make_shared<GameObject>();
			}
			
			auto meshrenderer = object->AddComponent<MeshRenderer>();
			auto transform = object->AddComponent<Transform>();
			auto animator = object->AddComponent<Animator>();
			meshrenderer->SetMesh(*coreRef, L"../FBXOutput/knight5");

			Protocol::Vec3 pos = *add.mutable_pos();

			transform->SetPosition(pos.x(), pos.y(), pos.z());
			transform->SetRotation(-1.57f, 0.f, 0.f);
			transform->SetScale(0.01f, 0.01f, 0.01f);

			coreRef->FlushCommandQueue();
			coreRef->ResetCommandQueue();

			meshrenderer->ReleaseUploadBuffers();

			AddGameObject(object);
		}
		break;
	}
	case Protocol::PacketType::SC_MOVE_OBJECT: {
		OutputDebugStringA("SC_MOVE_OBJECT apcket received\n");

		Protocol::SC_MOVE_PACKET move;
		if (move.ParseFromArray(packet.body().data(), packet.body().size())) {
			auto it = _objects.find(packet.header().sessionid());
			if (it != _objects.end()) {
			}
		}
		break;
	}
	case Protocol::PacketType::SC_REMOVE: {
		OutputDebugStringA("SC_REMOVE apcket received\n");
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
}

void ServerTestScene::UpdateScene(const float deltaTime)
{
	for (auto& [id, obj] : _objects) {
		obj->Update(deltaTime);
	}
}

void ServerTestScene::RenderScene()
{
	for (auto& [id, obj] : _objects) {
		if (auto meshRenderer = obj->GetComponent<MeshRenderer>()) {
			meshRenderer->Render(*coreRef);
		}
	}
}

int ServerTestScene::GetSceneWidth() const
{
	return 0;
}

void ServerTestScene::RequestSceneChange()
{
	if (GET(Input).GetKeyDown(VK_TAB))
	{
		if (sManagerRef)
			sManagerRef->RequestSceneChange(SceneType::Start);
	}
}