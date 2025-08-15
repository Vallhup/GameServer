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
    int useInstancing;
    int hasAlpha;
    int padding;
};

struct AnimationConstants
{
    int boneCount;
    int currentFrame;
    int nextFrame;
    float ratio;
    int animationOffset;

    int isBlending;
    int prevCurrentFrame;
    int prevNextFrame;
    float prevRatio;
    int prevAnimationOffset;
    float blendRatio;

    float padding;
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

private:
    unique_ptr<Device> device;
    shared_ptr<SwapChain> swapChain;
    unique_ptr<CommandQueue> cmdQueue;
    unique_ptr<RootSignature> rootSig;
    unique_ptr<Shader> shader;
    unique_ptr<UploadBuffer> frameCB;
    unique_ptr<UploadBuffer> sceneCB;
    unique_ptr<UploadBuffer> animationCB;
    unique_ptr<DepthStencilBuffer> depthStencilBuffer;
    unique_ptr<DescriptorHeap> descriptorHeap;
};
