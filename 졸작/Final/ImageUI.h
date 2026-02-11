#pragma once
#include "UIComponent.h"

enum class ImageUIState {
	Hidden,
	FadingIn,
	Visible,
	Pulsing,
	FadingOut,
};

class ImageUI : public UIComponent
{
public:
	ImageUI(const wstring& name, ImageUIState s);

	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetFadeDuration(float duration) { fadeDuration = duration; }
	void SetHoriLength(float length) { horizontalLength = length; }
	void SetVertLength(float length) { verticalLength = length; }
	void SetPulseSpeed(float speed) { pulseSpeed = speed; }

	void ChangeState(ImageUIState newState);
	ImageUIState GetState() const { return state; }

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
};

