#pragma once
#include "UIController.h"

class PanelUI;
class TextUI;

class GameSceneUIController : public UIController
{
public:
	void Init(UIManager* manager) override;
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

private:
	shared_ptr<PanelUI> statusPanel;
	shared_ptr<TextUI> testText;
};

