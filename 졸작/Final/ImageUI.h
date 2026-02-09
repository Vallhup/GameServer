#pragma once
#include "UIComponent.h"

// 오로지 Image 기반, Text 없음
class ImageUI : public UIComponent
{
public:
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

private:
	float fadeAlpha = 0.0f;
	float fadeDuration = 2.0f;
	float fadeElapsed = 0.0f;
	bool fading = false;
};

