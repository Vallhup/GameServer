#include "pch.h"
#include "UIComponent.h"
#include "UIManager.h"

void UIComponent::SetPosition(float x, float y)
{
	posX = basePosX = x;
	posY = basePosY = y;
}

void UIComponent::SetScale(float scl)
{
	scale = scl;
}
