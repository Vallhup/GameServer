#pragma once

enum class PSOType {
	Opaque,
	Transparent,
	GBuffer,
	Lighting,
	Compute,
	Shadow,
	SSAO,
	END
};

enum class ShaderType {
	ForwardVS, ForwardPS,
	GBufferVS, GBufferPS,
	FullscreenVS, LightingPS,
	AnimationCS,
	ShadowVS, ShadowPS,
	SSAOVS, SSAOPS,
	END
};

class Shader
{
public:
	void InitializeAllShaders(ID3D12Device* device, ID3D12RootSignature* rootSig);

	void InitializeForwardShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeGBufferShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeLightingShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeComputeShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath);
	void InitializeShadowShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeSSAOShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);

	ID3D12PipelineState* GetPSO(PSOType type) const;

private:
	void CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut);

private:
	array<ComPtr<ID3D12PipelineState>, static_cast<size_t>(PSOType::END)> mPSOs;
	array<ComPtr<ID3DBlob>, static_cast<size_t>(ShaderType::END)> mShadersBlobs;
};

