#pragma once

class DX12Core;
class GameObject;
class Mesh;
class Animator;
class Camera;
class Terrain;
class Water;
class InstancingBatch;
struct ObjectConstants;

class SceneRenderer
{
public:
    void Initialize(ID3D12Device* device);
    void BeginFrame();

    void RenderDeferred(DX12Core& core, const vector<shared_ptr<GameObject>>& objects, const Camera* cam);
    void RenderShadow(DX12Core& core, const vector<shared_ptr<GameObject>>& objects);
    void RenderTerrain(DX12Core& core, Terrain* terrain);
    void RenderWater(DX12Core& core, Water* water);
    void RenderInstanced(DX12Core& core, Mesh* mesh, UINT instanceCount, UploadBuffer* instanceBuffer, InstancingBatch* batch);
    void RenderInstancedShadow(DX12Core& core, Mesh* mesh, UINT instanceCount, UploadBuffer* instanceBuffer, InstancingBatch* batch);
    void RenderCollisionMeshWireframe(DX12Core& core, const vector<shared_ptr<GameObject>>& objects);


    void ReleaseUploadBuffer();

private:
    void SetupRenderingState(DX12Core& core, UploadBuffer* instanceBuffer = nullptr);
    ObjectConstants MakeObjectConstants(const XMMATRIX& world, int hasTexture, int doInstancing, UINT matIndex);

private:
    unique_ptr<UploadBuffer> objectCBPool;
    UINT cbIndex = 0;

    static constexpr size_t MAX_OBJECTS = 5000;
};