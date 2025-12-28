#include "pch.h"
#include "Shader.h"

void Shader::InitializeAllShaders(ID3D12Device* device, ID3D12RootSignature* rootSig)
{
    InitializeForwardShader(device, rootSig, L"../Shaders/ForwardVS.hlsli", L"../Shaders/ForwardPS.hlsli");
    InitializeGBufferShader(device, rootSig, L"../Shaders/GBufferVS.hlsli", L"../Shaders/GBufferPS.hlsli");
    InitializeLightingShader(device, rootSig, L"../Shaders/FullscreenVS.hlsli", L"../Shaders/LightingPS.hlsli");
    InitializeComputeShader(device, rootSig, L"../Shaders/Animation.hlsli");
    InitializeShadowShader(device, rootSig, L"../Shaders/ShadowVS.hlsli", L"../Shaders/ShadowPS.hlsli");
    InitializeDebugLinePSO(device, rootSig);
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
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
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

    psoDesc.NumRenderTargets = 4;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.RTVFormats[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::GBuffer)]));
    MASSERT(SUCCEEDED(hr), "Failed to create GBuffer PSO");

    OutputDebugStringA("G-Buffer PSO created!!\n");
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
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  

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

void Shader::InitializeComputeShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath)
{
    CompileShader(csPath, "CSMain", "cs_5_1", mShadersBlobs[static_cast<size_t>(ShaderType::AnimationCS)]);

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
    computePsoDesc.pRootSignature = rootSig;
    computePsoDesc.CS = { mShadersBlobs[static_cast<size_t>(ShaderType::AnimationCS)]->GetBufferPointer(), mShadersBlobs[static_cast<size_t>(ShaderType::AnimationCS)]->GetBufferSize() };

    HRESULT hr = device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::Compute)]));
    MASSERT(SUCCEEDED(hr), "Failed to create Compute PSO");
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
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
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
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[static_cast<size_t>(PSOType::DebugLine)]));
    MASSERT(SUCCEEDED(hr), "Failed to create DebugLine PSO");

    OutputDebugStringA("DebugLine PSO created!\n");
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