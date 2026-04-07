#pragma once

enum class PSOType {
	Opaque,
	Transparent,
	GBuffer,
	GBufferNonCulling,
	GBufferInstancing,
	GBufferWireframe,
	Lighting,
	Compute,
	Shadow,
	DebugLine,
	Skybox,
	Ssao,
	SsaoBlur,
	Trail,
	Flame,
	Spark,
	END
};

enum class ShaderType {
	ForwardVS, ForwardPS,
	GBufferVS, GBufferPS,
	FullscreenVS, LightingPS,
	AnimationCS,
	ShadowVS, ShadowPS,
	SkyboxVS, SkyboxPS,
	SsaoPS, SsaoBlurPS,
	EffectVS,
	TrailPS,
	FlamePS,
	SparkPS,
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
	void InitializeSsaoShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeSsaoBlurShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath);
	void InitializeEffectVS(ID3D12Device* device, const wstring& vsPath);
	void CreateEffectPSO(ID3D12Device* device, ID3D12RootSignature* rootSig, ShaderType psType, PSOType psoType, const wstring& psPath);

	void CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut);

private:
	array<ComPtr<ID3D12PipelineState>, static_cast<size_t>(PSOType::END)> mPSOs;
	array<ComPtr<ID3DBlob>, static_cast<size_t>(ShaderType::END)> mShadersBlobs;
};

