#include "pch.h"
#include "PvpInstance.h"

void PvpInstance::Update(float deltaTime)
{
	// TODO : Object Update
}

void PvpInstance::LoadStaticGameObject()
{
	// TEMP : 각 Instance에 맞는 Static Object Load

	ObjectId tempId{ 0, Static };
	auto temp = std::make_shared<GameObject>(tempId);
	_objMng->AddObject(temp);
}

void PvpInstance::Stop()
{
	// TODO : Resource 정리, 상위 Class (GameWorld or IGameContext)에 삭제 알림
}