#pragma once
#include "Scene.h"

class LoginScene final : public Scene
{
public:
	LoginScene() = default;
	LoginScene(const LoginScene&) = delete;
	LoginScene& operator=(const LoginScene&) = delete;
	~LoginScene();

	void Release() override;
	void Reset() override;

protected:
	const float* GetBackgroundColor() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderSceneDeferred() override;
	void RenderSceneForward() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:

};
