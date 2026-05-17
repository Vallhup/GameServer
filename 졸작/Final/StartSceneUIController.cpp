#include "pch.h"
#include "StartSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Engine.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "SoundManager.h"

void StartSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	InitMainImage();
	InitPressAnyButton();
	InitMenuButtons();
}

void StartSceneUIController::InitMainImage()
{
	mainImage = make_shared<ImageUI>(uiManager, L"MainPage", ImageUIState::FadingIn);
	mainImage->SetHoriLength(WinSize.x);
	mainImage->SetVertLength(WinSize.y);
	mainImage->SetFadeDuration(4.0f);
	widgets.push_back(mainImage);
}

void StartSceneUIController::InitPressAnyButton()
{
	pabImage = make_shared<ImageUI>(uiManager, L"PAB", ImageUIState::Hidden);
	pabImage->SetPosition((WinSize.x * 0.727f) / 2.f, WinSize.y * 0.7f);
	pabImage->SetHoriLength(WinSize.x * 0.273f);
	pabImage->SetVertLength(WinSize.y * 0.083f);
	widgets.push_back(pabImage);
}

void StartSceneUIController::InitMenuButtons()
{
	loginImage = make_shared<ImageUI>(uiManager, L"LOGIN", ImageUIState::Hidden);
	loginImage->SetPosition(WinSize.x * 0.3215f, WinSize.y * 0.7f);
	loginImage->SetHoriLength(WinSize.x * 0.117f);
	loginImage->SetVertLength(WinSize.y * 0.1f);
	loginImage->SetFadeDuration(2.0f);
	loginImage->SetHoverScale(1.1f);
	widgets.push_back(loginImage);

	exitImage = make_shared<ImageUI>(uiManager, L"EXIT", ImageUIState::Hidden);
	exitImage->SetPosition(WinSize.x * 0.5615f, WinSize.y * 0.7f);
	exitImage->SetHoriLength(WinSize.x * 0.117f);
	exitImage->SetVertLength(WinSize.y * 0.1f);
	exitImage->SetFadeDuration(2.0f);
	exitImage->SetHoverScale(1.1f);
	widgets.push_back(exitImage);
}

void StartSceneUIController::Update(float deltaTime)
{
	UIController::Update(deltaTime);

	if (mainImage->GetState() == ImageUIState::Visible &&
		pabImage->GetState() == ImageUIState::Hidden &&
		loginImage->GetState() == ImageUIState::Hidden)
	{
		pabImage->ChangeState(ImageUIState::Pulsing);
	}

	if (pabImage->GetState() == ImageUIState::Pulsing && INPUT.GetAnyKeyDown())
	{
		pabImage->ChangeState(ImageUIState::Hidden);
		loginImage->ChangeState(ImageUIState::FadingIn);
		exitImage->ChangeState(ImageUIState::FadingIn);
		return;
	}

	if (loginImage->GetState() == ImageUIState::FadingIn ||
		loginImage->GetState() == ImageUIState::Visible)
	{
		loginImage->SetHovered(loginImage->IsMouseInside());
	}

	if (exitImage->GetState() == ImageUIState::FadingIn ||
		exitImage->GetState() == ImageUIState::Visible)
	{
		exitImage->SetHovered(exitImage->IsMouseInside());
	}

	if (loginImage->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		IMGUI.ShowLoginWindow();
		OutputDebugStringA("loginImage clicked!!\n");
	}

	if (IMGUI.IsLoginSuccess())
	{
		{
			NETWORK_MANAGER->SendLoginPacket(IMGUI.GetLoginId(), IMGUI.GetLoginPw());
			OutputDebugStringA("CSLoginPacket has sent!!\n");
		}

		IMGUI.ResetLoginSuccess();
		SCENE_MANAGER->RequestSceneChange(SceneType::Select);
		OutputDebugStringA("Login success! Moving to Select scene.\n");
	}

	if (exitImage->IsHovered() && INPUT.GetMouseButtonDown(MouseButton::LEFT))
	{
		SOUND_MANAGER->PlaySFX("../Assets/Music/SFX/ButtonPress.mp3");
		DestroyWindow(ENGINE.GetHwnd());
		OutputDebugStringA("exitImage clicked!!\n");
	}
}

