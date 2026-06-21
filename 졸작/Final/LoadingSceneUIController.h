#pragma once
#include "UIController.h"

class ImageUI;
enum class SceneType;

class LoadingSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;

	void SetProgress(float progress);
	void SetTargetScene(SceneType type);
	void Reset();

private:
	void InitBackground();
	void InitLoadBar();
	void InitPressAnyButton();
	void InitShortcutHint();

	shared_ptr<ImageUI> mainImage;
	shared_ptr<ImageUI> loadBarBack;
	shared_ptr<ImageUI> loadBar;
	shared_ptr<ImageUI> loadArrow;
	shared_ptr<ImageUI> pab;
	shared_ptr<ImageUI> shortcutHint;

	float loadProgress = 0.0f;
	float loadBarMaxWidth = 0.0f;

	SceneType targetScene;
};

