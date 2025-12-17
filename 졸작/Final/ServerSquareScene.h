#pragma once
#include "Scene.h"

class ServerSquareScene final : public Scene
{
public:
	ServerSquareScene() = default;
	ServerSquareScene(const ServerSquareScene&) = delete;
	ServerSquareScene& operator=(const ServerSquareScene&) = delete;
	~ServerSquareScene();

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
	void InitializeSceneObjectPools() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	void RenderSceneShadow() override;
	void RenderSceneEffects() override;
	void RequestSceneChange() override;

private:
	shared_ptr<MainCharacter> knight;
};

