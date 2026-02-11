#include "pch.h"
#include "StartSceneUIController.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Engine.h"
#include "Input.h"

void StartSceneUIController::Init(UIManager* manager)
{
	uiManager = manager;

	// MainPage - 전체 화면 배경
	mainImage = make_shared<ImageUI>(L"MainPage", ImageUIState::FadingIn);
	mainImage->Init(uiManager, SceneType::Start);
	mainImage->SetHoriLength(WinSize.x);
	mainImage->SetVertLength(WinSize.y);

	// PAB - Press Any Button
	pabImage = make_shared<ImageUI>(L"PAB", ImageUIState::Hidden);
	pabImage->Init(uiManager, SceneType::Start);
	pabImage->SetPosition((WinSize.x * 0.727f) / 2.f, WinSize.y * 0.7f);
	pabImage->SetHoriLength(WinSize.x * 0.273f);
	pabImage->SetVertLength(WinSize.y * 0.083f);

	// LOGIN 버튼
	loginImage = make_shared<ImageUI>(L"LOGIN", ImageUIState::Hidden);
	loginImage->Init(uiManager, SceneType::Start);
	loginImage->SetPosition(WinSize.x * 0.3215f, WinSize.y * 0.7f);
	loginImage->SetHoriLength(WinSize.x * 0.117f);
	loginImage->SetVertLength(WinSize.y * 0.1f);

	// EXIT 버튼
	exitImage = make_shared<ImageUI>(L"EXIT", ImageUIState::Hidden);
	exitImage->Init(uiManager, SceneType::Start);
	exitImage->SetPosition(WinSize.x * 0.5615f, WinSize.y * 0.7f);
	exitImage->SetHoriLength(WinSize.x * 0.117f);
	exitImage->SetVertLength(WinSize.y * 0.1f);
}

void StartSceneUIController::Update(float deltaTime)
{
	// UI 업데이트
	if (mainImage) mainImage->Update(deltaTime);
	if (pabImage) pabImage->Update(deltaTime);
	if (loginImage) loginImage->Update(deltaTime);
	if (exitImage) exitImage->Update(deltaTime);

	// 상태 전환 로직
	// 1. mainImage FadeIn 완료 → pabImage Pulsing 시작
	if (mainImage->GetState() == ImageUIState::Visible &&
		pabImage->GetState() == ImageUIState::Hidden &&
		loginImage->GetState() == ImageUIState::Hidden)
	{
		pabImage->ChangeState(ImageUIState::Pulsing);
	}

	// 2. pabImage Pulsing 중 아무 키 입력 → LOGIN/EXIT 표시
	if (pabImage->GetState() == ImageUIState::Pulsing && INPUT.GetAnyKeyDown())
	{
		pabImage->ChangeState(ImageUIState::Hidden);
		loginImage->ChangeState(ImageUIState::Pulsing);
		exitImage->ChangeState(ImageUIState::Pulsing);
	}
}

void StartSceneUIController::Render(SpriteBatch* batch)
{
	if (mainImage) mainImage->Render(batch);
	if (pabImage) pabImage->Render(batch);
	if (loginImage) loginImage->Render(batch);
	if (exitImage) exitImage->Render(batch);
}
