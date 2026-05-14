#pragma once
#include <SpriteBatch.h>

class UIManager;
class UIComponent;

class UIController
{
public:
	virtual void Init(UIManager* manager) = 0;
	virtual void Update(float deltaTime);
	virtual void Render(SpriteBatch* batch);

protected:
	UIManager* uiManager;
	vector<shared_ptr<UIComponent>> widgets;
};

