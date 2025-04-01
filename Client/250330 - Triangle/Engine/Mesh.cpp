#include "pch.h"
#include "Mesh.h"
#include "Engine.h"

void Mesh::Init(vector<Vertex>& vec)
{
	_vertexCount = static_cast<uint32>(vec.size());
	uint32 bufferSize = _vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexBuffer));

	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	_vertexBuffer->Map(0, &readRange, &vertexDataBuffer);
	::memcpy(vertexDataBuffer, &vec[0], bufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.StrideInBytes = sizeof(Vertex);							// 정점 1개의 크기
	_vertexBufferView.SizeInBytes = bufferSize;									// 버퍼의 크기
}

void Mesh::Render()
{
	CMD_LIST->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);		// 정점들 연결 상태
	CMD_LIST->IASetVertexBuffers(0, 1, &_vertexBufferView);						// Slot: (0 ~ 15)
	CMD_LIST->DrawInstanced(_vertexCount, 1, 0, 0);
}

void Mesh::MakeTriangles(vector<Vertex>& v)
{
	Vertex data;
	data.pos = XMFLOAT3(0.0f, -0.75f, 0.5f);
	data.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(-0.8f, -0.75f, 0.5f);
	data.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(-0.4f, 0.0f, 0.5f);
	data.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(0.8f, -0.75f, 0.5f);
	data.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(0.0f, -0.75f, 0.5f);
	data.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(0.4f, 0.0f, 0.5f);
	data.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(0.4f, 0.0f, 0.5f);
	data.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(-0.4f, 0.0f, 0.5f);
	data.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	v.push_back(data);

	data.pos = XMFLOAT3(0.0f, 0.75f, 0.5f);
	data.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	v.push_back(data);
}
