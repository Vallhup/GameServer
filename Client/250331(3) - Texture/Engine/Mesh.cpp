#include "pch.h"
#include "Mesh.h"
#include "Engine.h"

void Mesh::Init()
{
	CreateVertexBuffer();
	CreateIndexBuffer();
}

void Mesh::Render()
{
	CMD_LIST->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);		// 정점들 연결 상태
	CMD_LIST->IASetVertexBuffers(0, 1, &_vertexBufferView);						// Slot: (0 ~ 15)
	CMD_LIST->IASetIndexBuffer(&_indexBufferView);

	// TODO
	// 1) Buffer에 데이터 세팅
	// 2) TableDescHeap에다가 CBV 전달
	// 3) 모두 세팅이 끝났으면 TableDescHeap 커밋
	D3D12_CPU_DESCRIPTOR_HANDLE handle = GEngine->GetConstantBuffer()->PushData(0, &_transform, sizeof(_transform));
	GEngine->GetTableDescHeap()->SetCBV(handle, CBV_REGISTER::b0);
	handle = GEngine->GetConstantBuffer()->PushData(0, &_transform, sizeof(_transform));
	GEngine->GetTableDescHeap()->SetCBV(handle, CBV_REGISTER::b1);
	handle = GEngine->GetConstantBuffer()->PushData(0, &_transform, sizeof(_transform));
	GEngine->GetTableDescHeap()->SetCBV(handle, CBV_REGISTER::b2);
	handle = GEngine->GetConstantBuffer()->PushData(0, &_transform, sizeof(_transform));
	GEngine->GetTableDescHeap()->SetCBV(handle, CBV_REGISTER::b3);
	handle = GEngine->GetConstantBuffer()->PushData(0, &_transform, sizeof(_transform));
	GEngine->GetTableDescHeap()->SetCBV(handle, CBV_REGISTER::b4);			// 실제로 지금 b0 ~ b1까지 사용하고 있음

	GEngine->GetTableDescHeap()->CommitTable();

	// CMD_LIST->DrawInstanced(_vertexCount, 1, 0, 0);			// Vertex data로 그리기
	CMD_LIST->DrawIndexedInstanced(_indexCount, 1, 0, 0, 0);	// Vertex + Index data로 그리기
}

void Mesh::MakeTriangle()
{
	Vertex data;

	data.pos = XMFLOAT3(0.2f, -0.15f, 0.5f);
	data.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	triangle.push_back(data);

	data.pos = XMFLOAT3(-0.2f, -0.15f, 0.5f);
	data.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	triangle.push_back(data);

	data.pos = XMFLOAT3(0.0f, 0.2f, 0.5f);
	data.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	triangle.push_back(data);
}

void Mesh::MakeRectangle()
{
	Vertex data;

	data.pos = XMFLOAT3(0.5f, -0.5f, 0.5f);
	data.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	rectangle.push_back(data);

	data.pos = XMFLOAT3(-0.5f, -0.5f, 0.5f);
	data.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	rectangle.push_back(data);

	data.pos = XMFLOAT3(-0.5f, 0.5f, 0.5f);
	data.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	rectangle.push_back(data);

	data.pos = XMFLOAT3(0.5f, 0.5f, 0.5f);
	data.color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	rectangle.push_back(data);
}

void Mesh::SetRectangleIdx()
{
	rectIdx.push_back(0);
	rectIdx.push_back(1);
	rectIdx.push_back(2);
	rectIdx.push_back(0);
	rectIdx.push_back(2);
	rectIdx.push_back(3);
}

void Mesh::CreateVertexBuffer()
{
	MakeRectangle();
	vector<Vertex> vec = rectangle;

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

void Mesh::CreateIndexBuffer()
{
	SetRectangleIdx();
	vector<uint32> idx = rectIdx;

	_indexCount = static_cast<uint32>(idx.size());
	uint32 bufferSize = _indexCount * sizeof(uint32);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));

	void* indexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	_indexBuffer->Map(0, &readRange, &indexDataBuffer);
	::memcpy(indexDataBuffer, &idx[0], bufferSize);
	_indexBuffer->Unmap(0, nullptr);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;							// 정점 1개의 크기
	_indexBufferView.SizeInBytes = bufferSize;
}
