#pragma once
#include "UIController.h"

class ImageUI;
enum class SceneType;

class LoadingSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetProgress(float progress);
	void SetTargetScene(SceneType type);

private:
	shared_ptr<ImageUI> mainImage;
	shared_ptr<ImageUI> loadBarBackImage;
	shared_ptr<ImageUI> loadBar;
	shared_ptr<ImageUI> loadArrow;

	float loadProgress = 0.0f;
	float loadBarMaxWidth = 0.0f;

	SceneType targetScene;
};

