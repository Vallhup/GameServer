#pragma once

class Shader
{
public:
	void Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);

	ID3D12PipelineState* GetOpaquePSO() const;
	ID3D12PipelineState* GetTransparentPSO() const;

private:
	void CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut);

private:
	ComPtr<ID3D12PipelineState> opaquePSO;      
	ComPtr<ID3D12PipelineState> transparentPSO;

	ComPtr<ID3DBlob> vertexshader;
	ComPtr<ID3DBlob> pixelshader;
};

