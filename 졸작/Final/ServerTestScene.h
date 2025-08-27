#pragma once

#include <unordered_map>
#include "Scene.h"

class ServerTestScene final : public Scene {
public:
	ServerTestScene() = default;
	~ServerTestScene() = default;

public:
	virtual void Release() override;
	virtual void Reset() override;

	void AddGameObject(const shared_ptr<GameObject>& obj);
	void HandlePacket(const Protocol::GamePacket& packet);

public:
	const float* GetBackgroundColor() override;
	void InitializeLogic() override;
	void UpdateScene(const float deltaTime) override;
	void RenderScene() override;
	int GetSceneWidth() const override;
	void RequestSceneChange() override;

private:
	unordered_map<int, shared_ptr<GameObject>> _objects;
};

