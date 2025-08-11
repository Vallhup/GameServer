#include "pch.h"
#include "VertexIndexBuffer.h"

void VertexIndexBuffer::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const vector<Vertex>& vertices, const vector<UINT>& indices)
{
    UINT vbSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());
    UINT ibSize = static_cast<UINT>(sizeof(UINT) * indices.size());
    indexcount = static_cast<UINT>(indices.size());

    CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);

    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &vbDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&vertexbuffer)
    )), "Failed to create VertexBuffer");

    CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexuploadbuffer)
    )), "Failed to create Vertex UploadBuffer");

    D3D12_SUBRESOURCE_DATA vbData = {};
    vbData.pData = vertices.data();
    vbData.RowPitch = vbSize;
    vbData.SlicePitch = vbSize;

    UpdateSubresources<1>(cmdList, vertexbuffer.Get(), vertexuploadbuffer.Get(), 0, 0, 1, &vbData);

    CD3DX12_RESOURCE_BARRIER vbBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        vertexbuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    cmdList->ResourceBarrier(1, &vbBarrier);

    vertexview.BufferLocation = vertexbuffer->GetGPUVirtualAddress();
    vertexview.StrideInBytes = sizeof(Vertex);
    vertexview.SizeInBytes = vbSize;

    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);

    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &ibDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&indexbuffer)
    )), "Failed to create IndexBuffer");

    MASSERT(SUCCEEDED(device->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexuploadbuffer)
    )), "Failed to create Index UploadBuffer");

    D3D12_SUBRESOURCE_DATA ibData = {};
    ibData.pData = indices.data();
    ibData.RowPitch = ibSize;
    ibData.SlicePitch = ibSize;

    UpdateSubresources<1>(cmdList, indexbuffer.Get(), indexuploadbuffer.Get(), 0, 0, 1, &ibData);

    CD3DX12_RESOURCE_BARRIER ibBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        indexbuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_INDEX_BUFFER
    );
    cmdList->ResourceBarrier(1, &ibBarrier);

    indexview.BufferLocation = indexbuffer->GetGPUVirtualAddress();
    indexview.SizeInBytes = ibSize;
    indexview.Format = DXGI_FORMAT_R32_UINT;
}

void VertexIndexBuffer::Bind(ID3D12GraphicsCommandList* cmdList)
{
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vertexview);
    cmdList->IASetIndexBuffer(&indexview);
}

void VertexIndexBuffer::Draw(ID3D12GraphicsCommandList* cmdList)
{
    cmdList->DrawIndexedInstanced(indexcount, 1, 0, 0, 0);
}

void VertexIndexBuffer::DrawInstanced(ID3D12GraphicsCommandList* cmdList, UINT instanceCount)
{
    cmdList->DrawIndexedInstanced(indexcount, instanceCount, 0, 0, 0);
}

void VertexIndexBuffer::DrawIndexed(ID3D12GraphicsCommandList* cmdList, UINT indexCount, UINT startIndex)
{
    cmdList->DrawIndexedInstanced(indexCount, 1, startIndex, 0, 0);
}
