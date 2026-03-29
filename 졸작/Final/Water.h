#pragma once

class DX12Core;
class VertexIndexBuffer;

class Water
{
public:
	Water() = default;
	~Water() = default;

	void Initialize(DX12Core& core);
	void Render(DX12Core& core, ID3D12GraphicsCommandList* cmdList);

	D3D12_GPU_VIRTUAL_ADDRESS GetCBAddress() const { return objectCB->GetGPUVirtualAddress(); }

	void SetPosition(float x, float y, float z);
	void SetScale(float x, float y, float z);

	void BindReflectionRT(ID3D12GraphicsCommandList* cmdList);
	void BindRefractionRT(ID3D12GraphicsCommandList* cmdList);

	D3D12_CPU_DESCRIPTOR_HANDLE GetReflectionRTV() const { return reflectionRTVHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetReflectionDSV() const { return reflectionDepthHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRefractionRTV() const { return refractionRTVHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRefractionDSV() const { return refractionDepthHandle; }

	ID3D12Resource* GetReflectionRT() const { return reflectionRT.Get(); }

private:
	void BuildVertices();
	void BuildIndices();

	void CreateReflectionResources(ID3D12Device* device);
	void CreateRefractionResources(ID3D12Device* device);
	void CreateRenderTargetView(ID3D12Device* device);

	void CopyBackBuffer(DX12Core& core);

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;

	vector<Vertex> vertices;
	vector<UINT> indices;

	unique_ptr<UploadBuffer> objectCB;

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	ComPtr<ID3D12Resource> reflectionRT;
	ComPtr<ID3D12Resource> reflectionDSVBuffer;			// (DSV만)
	ComPtr<ID3D12DescriptorHeap> reflectionDepthHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE reflectionDepthHandle = {};

	ComPtr<ID3D12DescriptorHeap> reflectionRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE reflectionRTVHandle = {};

	ComPtr<ID3D12DescriptorHeap> reflectionSRVHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE reflectionSRVHandle = {};

	ComPtr<ID3D12Resource> refractionRT;
	ComPtr<ID3D12Resource> refractionDepthTexture;		// (DSV + SRV)
	ComPtr<ID3D12DescriptorHeap> refractionDepthHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE refractionDepthHandle = {};

	ComPtr<ID3D12DescriptorHeap> refractionRTVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE refractionRTVHandle = {};

	ComPtr<ID3D12DescriptorHeap> refractionSRVHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE refractionSRVHandle = {};
};

