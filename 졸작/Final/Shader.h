#pragma once

enum class PSOType {
	Opaque,
	Transparent,
	GBuffer,
	GBufferInstancing,
	Lighting,
	Compute,
	Shadow,
	DebugLine,
	Skybox,
	END
};

enum class ShaderType {
	ForwardVS, ForwardPS,
	GBufferVS, GBufferPS,
	FullscreenVS, LightingPS,
	AnimationCS,
	ShadowVS, ShadowPS,
	SkyboxVS, SkyboxPS,
	END
};

class Shader
{
public:
	void InitializeAllShaders(ID3D12Device* device, ID3D12RootSignature* rootSig);
 
	ID3D12PipelineState* GetPSO(PSOType type) const;

private:
	void InitializeForwardShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeGBufferShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeLightingShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeComputeShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath);
	void InitializeShadowShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeDebugLinePSO(ID3D12Device* device, ID3D12RootSignature* rootSig);
	void InitializeSkyboxShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);

	void CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut);

private:
	array<ComPtr<ID3D12PipelineState>, static_cast<size_t>(PSOType::END)> mPSOs;
	array<ComPtr<ID3DBlob>, static_cast<size_t>(ShaderType::END)> mShadersBlobs;
};

