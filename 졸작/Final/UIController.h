#pragma once
#include <SpriteBatch.h>

class UIManager;

class UIController
{
public:
	virtual void Init(UIManager* manager) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render(SpriteBatch* batch) = 0;

protected:
	UIManager* uiManager;
};

