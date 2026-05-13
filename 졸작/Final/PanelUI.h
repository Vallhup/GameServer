#pragma once
#include "UIComponent.h"

// Status처럼 Image 기반, Text가 수치로 작성되는 UI
class PanelUI : public UIComponent
{
public:
	PanelUI(UIManager* manager, const wstring& name);

	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void Toggle();

private:
	wstring textureName;
	
	float fadeAlpha = 0.0f;
	float fadeDuration = 2.0f;
	float fadeElapsed = 0.0f;
	bool fading = false;
};

