#pragma once
#include "UIController.h"

class ImageUI;
class TextUI;

class LoadingSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetProgress(float progress);

private:
	shared_ptr<ImageUI> mainImage;
	shared_ptr<ImageUI> loadBarBackImage;

	shared_ptr<TextUI> progressText;

	float loadProgress = 0.0f;
};

