#pragma once
#include "UIComponent.h"

class TextUI : public UIComponent
{
public:
	TextUI(UIManager* manager, const wstring& name, const wstring& font);

	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetText(const wstring& lets) { letters = lets; }
	void SetTextColor(const XMVECTORF32& col) { color = col; }

private:
	wstring fontName;
	wstring letters;
	XMVECTORF32 color = Colors::White;
};

