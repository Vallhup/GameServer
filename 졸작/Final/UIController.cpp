#include "pch.h"
#include "UIController.h"
#include "UIManager.h"
#include "UIComponent.h"

void UIController::Update(float deltaTime)
{
	for (auto& w : widgets) w->Update(deltaTime);
}

void UIController::Render(SpriteBatch* batch)
{
	for (auto& w : widgets) w->Render(batch);
}
