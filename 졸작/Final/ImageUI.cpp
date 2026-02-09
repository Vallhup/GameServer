#include "pch.h"
#include "ImageUI.h"
#include "UIManager.h"
#include <DirectXHelpers.h>

ImageUI::ImageUI(const wstring& name) : textureName(name)
{
	uiName = name;
}

void ImageUI::Update(float deltaTime)
{
	if (fading)
	{
		fadeElapsed += deltaTime;
		fadeAlpha = clamp(fadeElapsed / fadeDuration, 0.0f, 1.0f);

		if (fadeAlpha >= 1.0f)
		{
			fading = false;
			if (onFadeComplete) onFadeComplete();
		}
	}

	if (pulsing && !fading)
	{
		pulseTime += deltaTime * pulseSpeed;
		float t = (sin(pulseTime) + 1.0f) / 2.0f;
		fadeAlpha = 0.1f + t * 0.9f;
	}
}

void ImageUI::Render(SpriteBatch* batch)
{
	if (firstRender)
	{
		firstRender = false;
		fading = true;
		fadeElapsed = 0.0f;
		fadeAlpha = 0.0f;
	}

	auto tex = uiManager->GetUITexture(textureName);
	if (!tex) return;

	auto heap = uiManager->GetUISrvHeap();

	XMUINT2 texSize = GetTextureSize(tex->resource.Get());
	RECT destRect = { static_cast<LONG>(posX), static_cast<LONG>(posY),
		static_cast<LONG>(posX + horizontalLength), static_cast<LONG>(posY + verticalLength)};
	XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, fadeAlpha);
	batch->Draw(heap->GetGpuHandle(tex->heapIndex), texSize, destRect, color);
}

void ImageUI::SetPulsing(bool enable)
{
	pulsing = enable; 

	if (enable) 
		pulseTime = XM_PIDIV2;
}
