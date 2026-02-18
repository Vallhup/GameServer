#include "pch.h"
#include "UIComponent.h"
#include "UIManager.h"

void UIComponent::Init(UIManager* manager)
{
	uiManager = manager;
}

void UIComponent::SetPosition(float x, float y)
{
	posX = basePosX = x;
	posY = basePosY = y;
}

void UIComponent::SetScale(float scl)
{
	scale = scl;
}
