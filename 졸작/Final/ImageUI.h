#pragma once
#include "UIComponent.h"

// 오로지 Image 기반, Text 없음
class ImageUI : public UIComponent
{
public:
	ImageUI(const wstring& name);

	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetFadeDuration(float duration) { fadeDuration = duration; }
	void SetHoriLength(float length) { horizontalLength = length; }
	void SetVertLength(float length) { verticalLength = length; }

	void SetOnFadeComplete(function<void()> callback) { onFadeComplete = callback; }

	void SetPulsing(bool enable);
	void SetPulseSpeed(float speed) { pulseSpeed = speed; }

private:
	wstring textureName;

	float fadeAlpha = 0.0f;
	float fadeDuration = 3.0f;
	float fadeElapsed = 0.0f;
	bool fading = false;

	bool firstRender = true;

	// ImageUI의 가로 & 세로 길이
	float horizontalLength = 0.0f;
	float verticalLength = 0.0f;

	function<void()> onFadeComplete;

	bool pulsing = false;
	float pulseSpeed = 2.0f;
	float pulseTime = 0.0f;
};

