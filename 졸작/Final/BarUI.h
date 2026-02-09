#pragma once
#include "UIComponent.h"

// 체력, 기력 등
class BarUI : public UIComponent
{
public:
	void Update(float deltaTime) override;
	void Render(SpriteBatch* batch) override;
};

