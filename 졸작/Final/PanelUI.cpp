#include "pch.h"
#include "PanelUI.h"
#include "UIManager.h"
#include <DirectXHelpers.h>

PanelUI::PanelUI(const wstring& name) : textureName(name) 
{
	uiName = name;
}

void PanelUI::Update(float deltaTime)
{
	if (fading)
	{
		fadeElapsed += deltaTime;
		fadeAlpha = clamp(fadeElapsed / fadeDuration, 0.0f, 1.0f);

		if (fadeAlpha >= 1.0f)
			fading = false;
	}
}

void PanelUI::Render(SpriteBatch* batch)
{
	auto tex = uiManager->GetUITexture(textureName);
	if (!tex) return;

	auto heap = uiManager->GetUISrvHeap();

	XMUINT2 texSize = GetTextureSize(tex->resource.Get());
	RECT destRect = { static_cast<LONG>(posX), static_cast<LONG>(posY), 
		static_cast<LONG>(texSize.x * scale + posX), static_cast<LONG>(texSize.y * scale + posY) };
	XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, fadeAlpha);
	batch->Draw(heap->GetGpuHandle(tex->heapIndex), texSize, destRect, color);
}

void PanelUI::Toggle()
{
	fadeElapsed = 0.0f;
	fadeAlpha = 0.0f;
	fading = true;
}
