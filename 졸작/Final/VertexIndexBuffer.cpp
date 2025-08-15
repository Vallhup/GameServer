#include "pch.h"
#include "VertexIndexBuffer.h"

void VertexIndexBuffer::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const vector<Vertex>& vertices, const vector<UINT>& indices)
{
    UINT vbSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());
    UINT ibSize = static_cast<UINT>(sizeof(UINT) * indices.size());
    indexCount = static_cast<UINT>(indices.size());

    CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);

    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &vbDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer)
    )), "Failed to create VertexBuffer");

    CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexUploadBuffer)
    )), "Failed to create Vertex UploadBuffer");

    D3D12_SUBRESOURCE_DATA vbData = {};
    vbData.pData = vertices.data();
    vbData.RowPitch = vbSize;
    vbData.SlicePitch = vbSize;

    UpdateSubresources<1>(cmdList, vertexBuffer.Get(), vertexUploadBuffer.Get(), 0, 0, 1, &vbData);

    CD3DX12_RESOURCE_BARRIER vbBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    cmdList->ResourceBarrier(1, &vbBarrier);

    vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexView.StrideInBytes = sizeof(Vertex);
    vertexView.SizeInBytes = vbSize;

    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);

    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &ibDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&indexBuffer)
    )), "Failed to create IndexBuffer");

    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexUploadBuffer)
    )), "Failed to create Index UploadBuffer");

    D3D12_SUBRESOURCE_DATA ibData = {};
    ibData.pData = indices.data();
    ibData.RowPitch = ibSize;
    ibData.SlicePitch = ibSize;

    UpdateSubresources<1>(cmdList, indexBuffer.Get(), indexUploadBuffer.Get(), 0, 0, 1, &ibData);

    CD3DX12_RESOURCE_BARRIER ibBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_INDEX_BUFFER
    );
    cmdList->ResourceBarrier(1, &ibBarrier);

    indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexView.SizeInBytes = ibSize;
    indexView.Format = DXGI_FORMAT_R32_UINT;
}

void VertexIndexBuffer::Bind(ID3D12GraphicsCommandList* cmdList)
{
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vertexView);
    cmdList->IASetIndexBuffer(&indexView);
}

void VertexIndexBuffer::Draw(ID3D12GraphicsCommandList* cmdList)
{
    cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void VertexIndexBuffer::DrawInstanced(ID3D12GraphicsCommandList* cmdList, UINT instancecount)
{
    cmdList->DrawIndexedInstanced(indexCount, instancecount, 0, 0, 0);
}

void VertexIndexBuffer::DrawIndexed(ID3D12GraphicsCommandList* cmdList, UINT indexcount, UINT startindex)
{
    cmdList->DrawIndexedInstanced(indexcount, 1, startindex, 0, 0);
}

void VertexIndexBuffer::ReleaseUploadBuffers()
{
    vertexUploadBuffer.Reset(); 
    indexUploadBuffer.Reset();
}