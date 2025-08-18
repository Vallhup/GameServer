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
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderScene() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:

};

