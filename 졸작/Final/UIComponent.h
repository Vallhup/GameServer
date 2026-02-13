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
	bool IsVisible() const { return visible; }
	SceneType GetOwnerSceneType() const { return ownerScene; }
	const wstring& GetUIName() const { return uiName; }

	void SetPosition(float x, float y);
	void SetScale(float scl);

protected:
	UIManager* uiManager = nullptr;
	SceneType ownerScene;

	// [LAW] uiName = TextureName
	wstring uiName;
	bool visible = false;

	// UI transform
	float posX = 0.f, posY = 0.f;
	float basePosX = 0.f, basePosY = 0.f;
	float scale = 1.f;
};

