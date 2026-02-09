#pragma once
#include "UIComponent.h"

// 오직 Text 기반 UI, Image 없음
class TextUI : public UIComponent
{
public:
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;
};

