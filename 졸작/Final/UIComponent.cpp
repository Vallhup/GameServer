#include "pch.h"
#include "UIComponent.h"
#include "UIManager.h"
#include "SceneManager.h"

void UIComponent::Init(UIManager* manager, SceneType scene)
{
	uiManager = manager;
	ownerScene = scene;
}
