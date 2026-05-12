#pragma once
#include "UIComponent.h"

enum class ImageUIState {
	Hidden,
	FadingIn,
	Visible,
	Pulsing,
	PulseOnce,
	FadingOut,
};

class ImageUI : public UIComponent
{
public:
	ImageUI(const wstring& name, ImageUIState s);

	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetFadeDuration(float duration) { fadeDuration = duration; }
	void SetHoriLength(float length) { horizontalLength = baseHoriLength = length; }
	void SetVertLength(float length) { verticalLength = baseVertLength = length; }
	void SetPulseSpeed(float speed) { pulseSpeed = speed; }
	void SetHoverScale(float scale) { hoverScale = scale; }
	void SetTintAlpha(float a) { tintAlpha = a; }

	void ChangeState(ImageUIState newState);
	ImageUIState GetState() const { return state; }

	bool IsMouseInside() const;
	void SetHovered(bool hover);
	bool IsHovered() const { return isHovered; };

private:
	void EnterState(ImageUIState newState);

	wstring textureName;
	ImageUIState state;

	float fadeAlpha = 0.0f;
	float fadeDuration = 3.0f;
	float fadeElapsed = 0.0f;

	float horizontalLength = 0.0f;
	float verticalLength = 0.0f;

	float pulseSpeed = 2.0f;
	float pulseTime = 0.0f;
	float holdTime = 0.0f;

	bool isHovered = false;
	float hoverScale = 1.15f;
	float baseHoriLength = 0.0f;
	float baseVertLength = 0.0f;

	float tintAlpha = 1.0f;
};

