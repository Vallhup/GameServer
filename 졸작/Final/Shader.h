#pragma once

class Shader
{
public:
	void InitializeForwardShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeGBufferShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeLightingShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeComputeShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath);
	void InitializeShadowShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);

	ID3D12PipelineState* GetOpaquePSO() const;
	ID3D12PipelineState* GetTransparentPSO() const;
	ID3D12PipelineState* GetGBufferPSO() const;
	ID3D12PipelineState* GetLightingPSO() const;
	ID3D12PipelineState* GetComputePSO() const;
	ID3D12PipelineState* GetShadowPSO() const;

private:
	void CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut);

private:
	// Forward
	ComPtr<ID3D12PipelineState> opaquePSO;      
	ComPtr<ID3D12PipelineState> transparentPSO;
	ComPtr<ID3DBlob> forwardVertexShader;
	ComPtr<ID3DBlob> forwardPixelShader;

	// Deferred 1Pass
	ComPtr<ID3D12PipelineState> gBufferPSO;
	ComPtr<ID3DBlob> gBufferVertexShader;
	ComPtr<ID3DBlob> gBufferPixelShader;

	// Deferred 2Pass
	ComPtr<ID3D12PipelineState> lightingPSO;
	ComPtr<ID3DBlob> lightingPixelShader;
	ComPtr<ID3DBlob> fullscreenVertexShader;

	// Compute
	ComPtr<ID3D12PipelineState> computePSO;
	ComPtr<ID3DBlob> computeShader;

	// Shadow
	ComPtr<ID3D12PipelineState> shadowPSO;
	ComPtr<ID3DBlob> shadowVertexShader;
	ComPtr<ID3DBlob> shadowPixelShader;
};

