#pragma once

// ============================================================================
// Orthodox SSAO Class
// Frank Luna 방식 기반 정통 SSAO 구현
// ============================================================================

class SSAO
{
public:
	// === 초기화 ===
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	// === Getters ===

	// SSAO 상태
	bool GetSSAOState() const { return enableSSAO; }
	void SetSSAOState(bool state) { enableSSAO = state; }

	// SSAO 결과 텍스처
	ID3D12Resource* GetSSAOTexture() const { return ssaoTexture.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSSAORTV() const { return ssaoRTV; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSSAOSRV() const { return ssaoSRV; }

	// View Space G-Buffer
	ID3D12Resource* GetViewNormal() const { return viewNormal.Get(); }
	ID3D12Resource* GetViewPosition() const { return viewPosition.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetViewNormalRTV() const { return viewNormalRTV; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetViewPositionRTV() const { return viewPositionRTV; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetViewNormalSRV() const { return viewNormalSRV; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetViewPositionSRV() const { return viewPositionSRV; }

	// Random Vector Texture
	ID3D12Resource* GetRandomTexture() const { return randomTexture.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetRandomSRV() const { return randomSRV; }

	// Offset Vectors (샘플 커널)
	void GetOffsetVectors(XMFLOAT4 outOffsets[14]) const;

private:
	// === 내부 초기화 함수들 ===
	void CreateSSAOTexture(ID3D12Device* device);
	void CreateViewSpaceGBuffer(ID3D12Device* device);
	void CreateRandomTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void CreateDescriptorHeaps(ID3D12Device* device);
	void CreateRTVs(ID3D12Device* device);
	void CreateSRVs(ID3D12Device* device);
	void BuildOffsetVectors();

	// === 리소스 ===

	// SSAO 결과 텍스처
	ComPtr<ID3D12Resource> ssaoTexture;

	// View Space G-Buffer (2개)
	ComPtr<ID3D12Resource> viewNormal;     // View Normal + Roughness
	ComPtr<ID3D12Resource> viewPosition;   // View Position + Depth

	// Random Vector Texture
	ComPtr<ID3D12Resource> randomTexture;
	ComPtr<ID3D12Resource> randomTextureUploadBuffer;

	// === Descriptor Heaps ===
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // RTV들 (SSAO + ViewNormal + ViewPos)
	ComPtr<ID3D12DescriptorHeap> srvHeap;  // SRV들 (SSAO + ViewNormal + ViewPos + Random)

	// === Descriptor Handles ===

	// SSAO
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoRTV;
	D3D12_GPU_DESCRIPTOR_HANDLE ssaoSRV;

	// View Space G-Buffer
	D3D12_CPU_DESCRIPTOR_HANDLE viewNormalRTV;
	D3D12_CPU_DESCRIPTOR_HANDLE viewPositionRTV;

	D3D12_GPU_DESCRIPTOR_HANDLE viewNormalSRV;
	D3D12_GPU_DESCRIPTOR_HANDLE viewPositionSRV;

	// Random Texture
	D3D12_GPU_DESCRIPTOR_HANDLE randomSRV;

	// === SSAO 파라미터 ===
	XMFLOAT4 offsetVectors[14];  // 샘플링 커널
	bool enableSSAO = false;
};