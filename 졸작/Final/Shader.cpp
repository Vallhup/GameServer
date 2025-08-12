#include "pch.h"
#include "Shader.h"

void Shader::Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_1", vertexShader);
    CompileShader(psPath, "PSMain", "ps_5_1", pixelShader);

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
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); 
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); 
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&opaquePSO));
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
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&transparentPSO));
    MASSERT(SUCCEEDED(hr), "Failed to create Transparent PSO");
}

void Shader::InitializeComputeShader(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& csPath)
{
    CompileShader(csPath, "CSMain", "cs_5_1", computeShader);

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
    computePsoDesc.pRootSignature = rootSig;
    computePsoDesc.CS = { computeShader->GetBufferPointer(), computeShader->GetBufferSize() };

    HRESULT hr = device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computePSO));
    MASSERT(SUCCEEDED(hr), "Failed to create Compute PSO");
}

ID3D12PipelineState* Shader::GetOpaquePSO() const
{
    return opaquePSO.Get();
}

ID3D12PipelineState* Shader::GetTransparentPSO() const
{
    return transparentPSO.Get();
}

ID3D12PipelineState* Shader::GetComputePSO() const
{
    return computePSO.Get();
}

void Shader::CompileShader(const wstring& path, const string& entry, const string& target, ComPtr<ID3DBlob>& blobOut)
{
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(
        path.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(),
        target.c_str(),
        0, 0,
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