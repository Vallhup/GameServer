#include "pch.h"
#include "Shader.h"

void Shader::InitializeAllShaders(ID3D12Device* device, ID3D12RootSignature* rootSig)
{
    InitializeForwardShader(device, rootSig, L"../Shaders/ForwardVS.hlsli", L"../Shaders/ForwardPS.hlsli");
    InitializeGBufferShader(device, rootSig, L"../Shaders/GBufferVS.hlsli", L"../Shaders/GBufferPS.hlsli");
    InitializeLightingShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/LightingPS.hlsli");
    InitializeComputeAnimationShader(device, rootSig, L"../Shaders/Animation.hlsli");
    InitializeClusterLightCullShader(device, rootSig, L"../Shaders/ClusterCullCS.hlsli");
    InitializeShadowShader(device, rootSig, L"../Shaders/ShadowVS.hlsli", L"../Shaders/ShadowPS.hlsli");
    InitializeDebugLinePSO(device, rootSig);
    InitializeSkyboxShader(device, rootSig, L"../Shaders/SkyboxVS.hlsli", L"../Shaders/SkyboxPS.hlsli");
    InitializeSsaoShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/SsaoPS.hlsli");
    InitializeSsaoBlurShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/SsaoBlurPS.hlsli");
    InitializeVolumetricFogPassShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/VolumetricFogPassPS.hlsli");
    InitializeBloomDownsampleShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/BloomDownsamplePS.hlsli");
    InitializeBloomUpsampleShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/BloomUpsamplePS.hlsli");
    InitializeBlitShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/BlitPS.hlsli");

    InitializeEffectVS(device, L"../Shaders/EffectVS.hlsli");
    CreateEffectPSO(device, rootSig, ShaderType::TrailPS, PSOType::Trail, L"../Shaders/TrailPS.hlsli");
    CreateEffectPSO(device, rootSig, ShaderType::FlamePS, PSOType::Flame, L"../Shaders/FlamePS.hlsli");
    CreateEffectPSO(device, rootSig, ShaderType::SparkPS, PSOType::Spark, L"../Shaders/SparkPS.hlsli");
    CreateEffectPSO(device, rootSig, ShaderType::GlowPS, PSOType::Glow, L"../Shaders/GlowPS.hlsli");
}

void Shader::InitializeForwardShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::ForwardVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::ForwardPS)]);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::ForwardVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::ForwardVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::ForwardPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::ForwardPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Opaque)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Opaque PSO");

    D3D12_BLEND_DESC transparentBlend = {};
    transparentBlend.RenderTarget[0].BlendEnable = TRUE;
    transparentBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparentBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparentBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    transparentBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    transparentBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    transparentBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparentBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC transparentDepth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    transparentDepth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; 

    psoDesc.BlendState = transparentBlend;
    psoDesc.DepthStencilState = transparentDepth;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Transparent)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Transparent PSO");
}

void Shader::InitializeGBufferShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::GBufferVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::GBufferPS)]);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::GBufferVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::GBufferVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::GBufferPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::GBufferPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 3;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::GBuffer)]));
    MASSERT(SUCCEEDED(hr), "Failed to create GBuffer PSO");

    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::GBufferNonCulling)]));
    MASSERT(SUCCEEDED(hr), "Failed to create GBufferNonCulling PSO");

    OutputDebugStringA("G-Buffer PSO created!!\n");

    // For instancing GBuffer PSO (CullMode = Back, Grass & Tree = None)
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::GBufferInstancing)]));
    MASSERT(SUCCEEDED(hr), "Failed to create GBufferInstancing PSO");

    // For GBuffer wireframe PSO (CullMode = NONE, FillMode = WIREFRAME)
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::GBufferWireframe)]));
    MASSERT(SUCCEEDED(hr), "Failed to create GBufferWireframe PSO");

    OutputDebugStringA("G-Buffer Instancing PSO created!!\n");
}

void Shader::InitializeLightingShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::LightingPS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };  
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::LightingPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::LightingPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;  
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Lighting)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Lighting PSO");

    OutputDebugStringA("Lighting PSO created successfully!\n");
}

void Shader::InitializeComputeAnimationShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath)
{
    CompileShader(csPath, "CSMain", "cs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::AnimationCS)]);

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
    computePsoDesc.pRootSignature = rootSig;
    computePsoDesc.CS = { mShadersBlobs[static_cast<size_t>(ShaderType::AnimationCS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::AnimationCS)]->GetBufferSize() };

    HRESULT hr = device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Compute)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Compute PSO");
}

void Shader::InitializeClusterLightCullShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath)
{
    CompileShader(csPath, "CSMain", "cs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::ClusterLightCullCS)]);

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
    computePsoDesc.pRootSignature = rootSig;
    computePsoDesc.CS = { mShadersBlobs[static_cast<size_t>(ShaderType::ClusterLightCullCS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::ClusterLightCullCS)]->GetBufferSize() };

    HRESULT hr = device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::ClusterLightCull)]));
    MASSERT(SUCCEEDED(hr), "Failed to create ClusterLightCull PSO");

    OutputDebugStringA("ClusterLightCull PSO created successfully!\n");
}

void Shader::InitializeShadowShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::ShadowVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::ShadowPS)]);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::ShadowVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::ShadowVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::ShadowPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::ShadowPS)]->GetBufferSize() };
    
    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;

    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 0;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Shadow)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Shadow PSO");
}

void Shader::InitializeDebugLinePSO(ID3D12Device* device, ID3D12RootSignature* rootSig)
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::ForwardVS)]->GetBufferPointer(),
                   mShadersBlobs[static_cast<size_t>(ShaderType::ForwardVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::ForwardPS)]->GetBufferPointer(),
                   mShadersBlobs[static_cast<size_t>(ShaderType::ForwardPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;  // ? LINE
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::DebugLine)]));
    MASSERT(SUCCEEDED(hr), "Failed to create DebugLine PSO");

    OutputDebugStringA("DebugLine PSO created!\n");
}

void Shader::InitializeSkyboxShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::SkyboxVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::SkyboxPS)]);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::SkyboxVS)]->GetBufferPointer(),
                   mShadersBlobs[static_cast<size_t>(ShaderType::SkyboxVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::SkyboxPS)]->GetBufferPointer(),
                   mShadersBlobs[static_cast<size_t>(ShaderType::SkyboxPS)]->GetBufferSize() };

    D3D12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_FRONT;
    psoDesc.RasterizerState = rasterDesc;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Skybox)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Skybox PSO");

    OutputDebugStringA("Skybox PSO created!\n");
}

void Shader::InitializeSsaoShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::SsaoPS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::SsaoPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::SsaoPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
    
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Ssao)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Ssao PSO");

    OutputDebugStringA("Ssao PSO created successfully!\n");
}

void Shader::InitializeSsaoBlurShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::SsaoBlurPS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::SsaoBlurPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::SsaoBlurPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::SsaoBlur)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Ssao Blur PSO");

    OutputDebugStringA("Ssao Blur PSO created successfully!\n");
}

void Shader::InitializeVolumetricFogPassShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::VolumetricFogPassPS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::VolumetricFogPassPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::VolumetricFogPassPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::VolumetricFogPass)]));
    MASSERT(SUCCEEDED(hr), "Failed to create VolumetricFogPass PSO");

    OutputDebugStringA("VolumetricFogPass PSO created successfully!\n");
}

void Shader::InitializeBlitShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::BlitPS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::BlitPS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::BlitPS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Blit)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Blit PSO");

    OutputDebugStringA("Blit PSO created successfully!\n");
}

void Shader::InitializeBloomDownsampleShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::BloomDownsamplePS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::BloomDownsamplePS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::BloomDownsamplePS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::BloomDownsample)]));
    MASSERT(SUCCEEDED(hr), "Failed to create BloomDownsample PSO");

    OutputDebugStringA("BloomDownsample PSO created successfully!\n");
}

void Shader::InitializeBloomUpsampleShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]);
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::BloomUpsamplePS)]);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::FullscreenVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(ShaderType::BloomUpsamplePS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::BloomUpsamplePS)]->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;

    // Additive blend: dst = src * 1 + dst * 1
    D3D12_BLEND_DESC additiveBlend = {};
    additiveBlend.RenderTarget[0].BlendEnable = TRUE;
    additiveBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    additiveBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    additiveBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    additiveBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    additiveBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    additiveBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    additiveBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState = additiveBlend;

    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::BloomUpsample)]));
    MASSERT(SUCCEEDED(hr), "Failed to create BloomUpsample PSO");

    OutputDebugStringA("BloomUpsample PSO created successfully!\n");
}

void Shader::InitializeEffectVS(ID3D12Device* device, const wstring& vsPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::EffectVS)]);
    OutputDebugStringA("Effect VS compiled!\n");
}

void Shader::CreateEffectPSO(ID3D12Device* device, ID3D12RootSignature* rootSig, ShaderType psType, PSOType psoType, const wstring& psPath)
{
    CompileShader(psPath, "PSMain", "ps_5_1", mShadersBlobs[static_cast<size_t>(psType)]);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "ALPHA", 0, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { mShadersBlobs[static_cast<size_t>(ShaderType::EffectVS)]->GetBufferPointer(),
                   mShadersBlobs[static_cast<size_t>(ShaderType::EffectVS)]->GetBufferSize() };
    psoDesc.PS = { mShadersBlobs[static_cast<size_t>(psType)]->GetBufferPointer(),
                   mShadersBlobs[static_cast<size_t>(psType)]->GetBufferSize() };

    D3D12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState = rasterDesc;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState = blendDesc;

    D3D12_DEPTH_STENCIL_DESC depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState = depthDesc;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(psoType)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Effect PSO");

    OutputDebugStringA("Effect PSO created!\n");
}

ID3D12PipelineState* Shader::GetPSO(PSOType type) const
{
    return mPSOs[static_cast<size_t>(type)].Get();
}

void Shader::CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut)
{
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    compileFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

    HRESULT hr = D3DCompileFromFile(
        path.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(),
        target.c_str(),
        compileFlags, 0,
        &blobOut,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());

        MASSERT(false, "Shader compile failed");
    }
}