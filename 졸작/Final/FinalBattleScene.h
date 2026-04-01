#pragma once
#include "Scene.h"

class FinalBattleScene final : public Scene
{
public:
	FinalBattleScene() = default;
	FinalBattleScene(const FinalBattleScene&) = delete;
	FinalBattleScene& operator=(const FinalBattleScene&) = delete;
	~FinalBattleScene() = default;

	void Release() override;
	void Reset() override;

protected:
	void InitializeSceneObjectPools() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;

};

