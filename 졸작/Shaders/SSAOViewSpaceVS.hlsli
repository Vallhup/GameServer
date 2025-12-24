// ============================================================================
// SSAO View Space G-Buffer - Vertex Shader
// 기존 G-Buffer에서 World Space 데이터를 읽어 View Space로 변환
// ============================================================================

#include "ShaderResources.hlsli"

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VSMain(uint vertexID : SV_VertexID)
{
    VS_OUT output;
    
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    
    output.pos = float4(
        output.uv.x * 2.0 - 1.0, 
        1.0 - output.uv.y * 2.0, 
        0.0,
        1.0
    );
    
    return output;
}
