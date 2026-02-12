#include "pch.h"
#include "ImageUI.h"
#include "UIManager.h"
#include "Input.h"
#include <DirectXHelpers.h>

ImageUI::ImageUI(const wstring& name, ImageUIState s) : textureName(name), state(s)
{
	uiName = name;
	EnterState(state);
}

void ImageUI::Update(float deltaTime)
{
	switch (state)
	{
	case ImageUIState::Hidden:
		break;

	case ImageUIState::FadingIn:
		fadeElapsed += deltaTime;
		fadeAlpha = clamp(fadeElapsed / fadeDuration, 0.0f, 1.0f);
		if (fadeAlpha >= 1.0f)
		{
			ChangeState(ImageUIState::Visible);
		}
		break;

	case ImageUIState::Visible:
		break;

	case ImageUIState::Pulsing:
		pulseTime += deltaTime * pulseSpeed;
		fadeAlpha = 0.01f + ((sin(pulseTime) + 1.0f) / 2.0f) * 0.99f;
		break;

	case ImageUIState::FadingOut:
		fadeElapsed += deltaTime;
		fadeAlpha = 1.0f - clamp(fadeElapsed / fadeDuration, 0.0f, 1.0f);
		if (fadeAlpha <= 0.0f)
		{
			ChangeState(ImageUIState::Hidden);
		}
		break;
	}
}

void ImageUI::Render(SpriteBatch* batch)
{
	if (state == ImageUIState::Hidden) return;

	auto tex = uiManager->GetUITexture(textureName);
	if (!tex) return;

	auto heap = uiManager->GetUISrvHeap();

	XMUINT2 texSize = GetTextureSize(tex->resource.Get());
	RECT destRect = { static_cast<LONG>(posX), static_cast<LONG>(posY),
		static_cast<LONG>(posX + horizontalLength), static_cast<LONG>(posY + verticalLength)};
	XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, fadeAlpha);
	batch->Draw(heap->GetGpuHandle(tex->heapIndex), texSize, destRect, color);
}

void ImageUI::ChangeState(ImageUIState newState)
{
	if (state == newState) return;
	state = newState;
	EnterState(newState);
}

bool ImageUI::IsMouseInside() const
{
	const XMFLOAT2& mousePos = INPUT.GetMousePosition();
	return (mousePos.x >= posX && mousePos.x <= posX + horizontalLength &&
			mousePos.y >= posY && mousePos.y <= posY + verticalLength);
}

void ImageUI::SetHovered(bool hover)
{
	if (isHovered == hover) return;
	isHovered = hover;

	if (isHovered)
	{
		ChangeState(ImageUIState::Visible);

		float newWidth = baseHoriLength * hoverScale;
		float newHeight = baseVertLength * hoverScale;
		posX = basePosX - (newWidth - baseHoriLength) / 2.0f;
		posY = basePosY - (newHeight - baseVertLength) / 2.0f;
		horizontalLength = newWidth;
		verticalLength = newHeight;
	}
	else
	{
		ChangeState(ImageUIState::Visible);

		posX = basePosX;
		posY = basePosY;
		horizontalLength = baseHoriLength;
		verticalLength = baseVertLength;
	}
}

void ImageUI::EnterState(ImageUIState newState)
{
	switch (newState)
	{
	case ImageUIState::Hidden:
		fadeAlpha = 0.0f;
		visible = false;
		break;

	case ImageUIState::FadingIn:
		fadeElapsed = 0.0f;
		fadeAlpha = 0.0f;
		visible = true;
		break;

	case ImageUIState::Visible:
		fadeAlpha = 1.0f;
		visible = true;
		break;

	case ImageUIState::Pulsing:
		pulseTime = -XM_PIDIV2;
		visible = true;
		break;

	case ImageUIState::FadingOut:
		fadeElapsed = 0.0f;
		fadeAlpha = 1.0f;
		visible = true;
		break;
	}
}
