#pragma once
#include <SpriteBatch.h>

class UIManager;

class UIComponent
{
public:
	virtual void Init(UIManager* manager);
	virtual void Update(float deltaTime) = 0;
	virtual void Render(SpriteBatch* batch) = 0;

	const wstring& GetUIName() const { return uiName; }

	void SetPosition(float x, float y);
	void SetScale(float scl);

	float GetPosX() const { return posX; }
	float GetPosY() const { return posY; }

protected:
	UIManager* uiManager = nullptr;

	// [LAW] uiName = TextureName
	wstring uiName;

	// UI transform
	float posX = 0.f, posY = 0.f;
	float basePosX = 0.f, basePosY = 0.f;
	float scale = 1.f;
};

