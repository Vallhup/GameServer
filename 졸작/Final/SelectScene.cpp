#include "pch.h"
#include "SelectScene.h"
#include "Engine.h"
#include "SoundManager.h"

void SelectScene::Release()
{
	SOUND_MANAGER->StopBGM(1.0f);

	OutputDebugStringA("SelectScene Data has been deleted!! \n----------------------------------------\n");
}

const char* SelectScene::GetBGMPath() const
{
	return "../Assets/Music/BGM/SelectBGM.mp3";
}

float SelectScene::GetBGMFadeInSeconds() const
{
	return 0.5f;
}

void SelectScene::InitializeLogic()
{
	OutputDebugStringA("----------------------------------------\nSelectScene Data has been created!! \n");
}
