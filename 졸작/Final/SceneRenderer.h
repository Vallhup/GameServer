#pragma once

class DX12Core;
class GameObject;
class Mesh;
class Animator;
struct ObjectConstants;

class SceneRenderer
{
public:
    void Initialize(ID3D12Device* device);

    void RenderDeferred(DX12Core& core, const vector<shared_ptr<GameObject>>& objects);
    void RenderForward(DX12Core& core, const vector<shared_ptr<GameObject>>& objects);
    void RenderShadow(DX12Core& core, const vector<shared_ptr<GameObject>>& objects);
    void RenderInstanced(DX12Core& core, Mesh* mesh, UINT instanceCount, UploadBuffer* instanceBuffer);

private:
    void SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer = nullptr);
    ObjectConstants MakeObjectConstants(const XMMATRIX& world, int hasTexture, int doInstancing, UINT matIndex);

private:
    unique_ptr<UploadBuffer> objectCBPool;
    static constexpr size_t MAX_OBJECTS = 1000;
    static constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;
};