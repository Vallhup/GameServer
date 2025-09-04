#pragma once

class DX12Core;
class SceneManager;
enum class SceneType;
class GameObject;
class MainCharacter;
class Camera;

class Scene
{
public:
	virtual ~Scene() {}
    virtual void Initialize(DX12Core& core);
    virtual void Update(const float deltaTime);
    virtual void RenderDeferred();
	virtual void RenderForward();
	virtual void RenderEffects();
    virtual void Release() = 0;
    virtual void Reset() = 0;

	virtual const float* GetBackgroundColor() = 0;

	Camera* GetCamera() const;
	void SetSceneManager(SceneManager* manager);

protected:
	virtual void InitializeLogic() = 0;
	virtual void UpdateScene(const float deltaTime) = 0;
	virtual void RenderSceneDeferred() = 0;
	virtual void RenderSceneForward() = 0;
	virtual void RenderSceneEffects() = 0;
	virtual int GetSceneWidth() const = 0;
	virtual void RequestSceneChange() = 0;

protected:
	XMFLOAT4X4 mView = {};
	XMFLOAT4X4 mProjection = {};

	DX12Core* coreRef = nullptr;
	SceneManager* sManagerRef = nullptr;

	unique_ptr<Camera> cam;
};

