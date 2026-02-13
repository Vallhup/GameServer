#pragma once
#include "UIComponent.h"

// 고정 위치 Text 기반 UI, Image 없음
class TextUI : public UIComponent
{
public:
	TextUI(const wstring& name, const wstring& font);

	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;

	void SetText(const wstring& lets) { letters = lets; }
	void SetTextColor(const XMVECTORF32& col) { color = col; }

private:
	// 폰트
	wstring fontName;
	// 화면에 출력할 글자들
	wstring letters;
	XMVECTORF32 color = Colors::White;
};

