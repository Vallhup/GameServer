#pragma once

class Device;
class SwapChain;
class CommandQueue;
class RootSignature;
class Shader;
class UploadBuffer;
class DepthStencilBuffer;
class VertexIndexBuffer;
class DescriptorHeap;
class Texture;

struct ObjectConstants
{
    XMMATRIX world;
    int useTexture;
    float heightScale;
    int useInstancing;
    int hasAlpha;
};

struct AnimationConstants
{
    int boneCount;
    int currentFrame;
    int nextFrame;
    float ratio;
};

class DX12Graphics
{
public:
	static DX12Graphics& Get();

	void Initialize(HWND hwnd);
    void FlushCommandQueue();
    void ResetCommandQueue();

    void RenderBegin(D3D12_VIEWPORT viewport, D3D12_RECT scissorRect);
    void RenderEnd();

    Device* GetDevice() const;
    CommandQueue* GetCmdQueue() const;
    RootSignature* GetRootSig() const;
    Shader* GetShader() const;
    UploadBuffer* GetFrameCB() const;
    UploadBuffer* GetSceneCB() const;
    UploadBuffer* GetAnimationCB() const;
    DescriptorHeap* GetDescHeap() const;
    Texture* GetGroundTexture() const;
    Texture* GetHeightMapTexture() const;

private:
    unique_ptr<Device> device;
    shared_ptr<SwapChain> swapchain;
    unique_ptr<CommandQueue> cmdQueue;
    unique_ptr<RootSignature> rootSig;
    unique_ptr<Shader> shader;
    unique_ptr<UploadBuffer> frameCB;
    unique_ptr<UploadBuffer> sceneCB;
    unique_ptr<UploadBuffer> animationCB;
    unique_ptr<DepthStencilBuffer> depthstencilbuffer;
    unique_ptr<DescriptorHeap> descriptorheap;
    unique_ptr<Texture> groundtexture;
    unique_ptr<Texture> heighttexture;
};
