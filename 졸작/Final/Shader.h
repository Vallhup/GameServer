#pragma once

class Shader
{
public:
	void Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);

	void InitializeGBufferShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeLightingShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);

	void InitializeComputeShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath);

	ID3D12PipelineState* GetOpaquePSO() const;
	ID3D12PipelineState* GetTransparentPSO() const;

	ID3D12PipelineState* GetGBufferPSO() const;
	ID3D12PipelineState* GetLightingPSO() const;

	ID3D12PipelineState* GetComputePSO() const;

private:
	void CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut);

private:
	ComPtr<ID3D12PipelineState> opaquePSO;      
	ComPtr<ID3D12PipelineState> transparentPSO;
	
	ComPtr<ID3D12PipelineState> gBufferPSO;
	ComPtr<ID3D12PipelineState> lightingPSO;

	ComPtr<ID3D12PipelineState> computePSO;

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

	ComPtr<ID3DBlob> gBufferVertexShader;
	ComPtr<ID3DBlob> gBufferPixelShader;
	ComPtr<ID3DBlob> lightingPixelShader;
	ComPtr<ID3DBlob> fullscreenVertexShader;

	ComPtr<ID3DBlob> computeShader;
};

