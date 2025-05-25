#pragma once

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

class VertexIndexBuffer
{
public:
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const vector<Vertex>& vertices, const vector<UINT>& indices);

    void Bind(ID3D12GraphicsCommandList* cmdList);
    void Draw(ID3D12GraphicsCommandList* cmdList);

private:
    ComPtr<ID3D12Resource> vertexbuffer;
    ComPtr<ID3D12Resource> vertexuploadbuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexview = {};

    ComPtr<ID3D12Resource> indexbuffer;
    ComPtr<ID3D12Resource> indexuploadbuffer;
    D3D12_INDEX_BUFFER_VIEW indexview = {};

    UINT indexcount = 0;
};
