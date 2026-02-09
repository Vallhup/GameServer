#pragma once
#include <SpriteBatch.h>

class UIManager;
enum class SceneType;

class UIComponent
{
public:
	virtual void Init(UIManager* manager, SceneType scene);
	virtual void Update(float deltaTime) = 0;
	virtual void Render(SpriteBatch* batch) = 0;

	void SetVisible(bool in) { visible = in; }
	bool Isvisible() const { return visible; }
	SceneType GetOwnerSceneType() const { return ownerScene; }

protected:
	UIManager* uiManager = nullptr;
	SceneType ownerScene;
	bool visible = true;
};

