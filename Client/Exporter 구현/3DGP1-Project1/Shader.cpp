#include "pch.h"
#include "Shader.h"

void Shader::Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig, const wstring& vsPath, const wstring& psPath)
{
    CompileShader(vsPath, "VSMain", "vs_5_0", vertexshader);
    CompileShader(psPath, "PSMain", "ps_5_0", pixelshader);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // 공통 PSO 설정
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = { vertexshader->GetBufferPointer(), vertexshader->GetBufferSize() };
    psoDesc.PS = { pixelshader->GetBufferPointer(), pixelshader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    // 1. 불투명 객체용 PSO
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // 블렌딩 비활성화
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // 깊이 쓰기 활성화
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&opaquePSO));
    MASSERT(SUCCEEDED(hr), "Failed to create Opaque PSO");

    // 2. 투명 객체용 PSO
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
    transparentDepth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 깊이 쓰기 비활성화

    psoDesc.BlendState = transparentBlend;
    psoDesc.DepthStencilState = transparentDepth;
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&transparentPSO));
    MASSERT(SUCCEEDED(hr), "Failed to create Transparent PSO");
}

ID3D12PipelineState* Shader::GetOpaquePSO() const
{
    return opaquePSO.Get();
}

ID3D12PipelineState* Shader::GetTransparentPSO() const
{
    return transparentPSO.Get();
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