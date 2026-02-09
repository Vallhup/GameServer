#pragma once
#include "UIComponent.h"

class ImageUI : public UIComponent
{
	float fadeAlpha = 0.0f;
	float fadeDuration = 2.0f;
	float fadeElapsed = 0.0f;
	bool fading = false;
	bool show = false;

public:
	void Update(float deltaTime) override;
	void Render() override;
};

