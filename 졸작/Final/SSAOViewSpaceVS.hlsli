// ============================================================================
// SSAO View Space G-Buffer - Vertex Shader
// 기존 G-Buffer에서 World Space 데이터를 읽어 View Space로 변환
// ============================================================================

cbuffer FrameCB : register(b0)
{
    matrix view;
    matrix projection;
    float3 cameraPosition;
    float framePadding;
};

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    
    // Fullscreen quad
    // 정점 위치: (-1,-1), (1,-1), (-1,1), (1,1) 등
    output.pos = float4(input.pos, 1.0f);
    output.uv = input.uv;
    
    return output;
}
