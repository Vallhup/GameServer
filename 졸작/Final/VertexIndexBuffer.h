#pragma once

class VertexIndexBuffer
{
public:
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const vector<Vertex>& vertices, const vector<UINT>& indices);

    void Bind(ID3D12GraphicsCommandList* cmdList);
    void Draw(ID3D12GraphicsCommandList* cmdList);
    void DrawInstanced(ID3D12GraphicsCommandList* cmdList, UINT instancecount);
    void DrawIndexed(ID3D12GraphicsCommandList* cmdList, UINT indexcount, UINT startindex);
    void DrawIndexedInstanced(ID3D12GraphicsCommandList* cmdList, UINT indexcount, UINT instancecount, UINT startindex);
    void ReleaseUploadBuffers();

private:
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> vertexUploadBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexView = {};

    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> indexUploadBuffer;
    D3D12_INDEX_BUFFER_VIEW indexView = {};

    UINT indexCount = 0;
};
