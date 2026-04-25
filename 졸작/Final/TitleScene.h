#pragma once
#include "Scene.h"

class TitleScene final : public Scene
{
public:
	TitleScene() = default;
	TitleScene(const TitleScene&) = delete;
	TitleScene& operator=(const TitleScene&) = delete;
	~TitleScene() = default;

	void Release() override;
	void Reset() override;

protected:
	void InitializeLogic() override;
	void InitializeSceneMonsters() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;
};
