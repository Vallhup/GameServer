#pragma once

class Scene
{
public:
	virtual ~Scene() {}
    virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    virtual void Update(const float deltaTime);
    virtual void Render();
    virtual void Release() = 0;
    virtual void Reset() = 0;

	virtual const float* GetBackgroundColor() = 0;

protected:
	virtual void InitializeLogic(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) = 0;
	virtual void UpdateLogic(const float deltaTime) = 0;
	virtual void RenderScene() = 0;
	virtual int GetSceneWidth() const = 0;

protected:
	XMFLOAT4X4 mView = {};
	XMFLOAT4X4 mProjection = {};
};

