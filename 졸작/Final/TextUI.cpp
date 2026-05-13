#include "pch.h"
#include "TextUI.h"
#include "UIManager.h"

TextUI::TextUI(UIManager* manager, const wstring& name, const wstring& font)
	: UIComponent(manager), fontName(font)
{
	uiName = name;
}

void TextUI::Update(float deltaTime)
{
}

void TextUI::Render(SpriteBatch* batch)
{
	auto fontData = uiManager->GetFont(fontName);
	if (!fontData) return;

	XMFLOAT2 pos(posX, posY);
	fontData->font->DrawString(batch, letters.c_str(), pos, color, 0.f, XMFLOAT2(0, 0), scale);
}
