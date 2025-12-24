// ============================================================================
// SSAO View Space G-Buffer - Pixel Shader
// World Space G-Buffer를 읽어서 View Space로 변환하여 저장
// ============================================================================

#include "ConstantBuffers.hlsli"

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PS_OUT
{
    float4 viewNormal : SV_Target0;    // View Space Normal (xyz) + Roughness (w)
    float4 viewPosition : SV_Target1;  // View Space Position (xyz) + Depth (w)
};

// 기존 World Space G-Buffer 읽기
Texture2D gBufferRT1 : register(t5); // World Normal + Roughness
Texture2D gBufferRT2 : register(t6); // World Position + AO

SamplerState pointSampler : register(s0);

PS_OUT PSMain(PS_IN input)
{
    PS_OUT output;
    
    // === 1. World Space G-Buffer 샘플링 ===
    float4 worldNormalRoughness = gBufferRT1.Sample(pointSampler, input.uv);
    float4 worldPosAO = gBufferRT2.Sample(pointSampler, input.uv);
    
    float3 worldNormal = worldNormalRoughness.xyz;
    float roughness = worldNormalRoughness.w;
    float3 worldPos = worldPosAO.xyz;
    
    // === 2. World Space → View Space 변환 ===
    
    // Position: World → View
    float4 viewPos4 = mul(float4(worldPos, 1.0f), view);
    float3 viewPos = viewPos4.xyz / viewPos4.w;
    
    // Normal: World → View (회전만, 이동 제외)
    float3 viewNormal = normalize(mul(worldNormal, (float3x3)view));
    
    // === 3. 출력 ===
    output.viewNormal = float4(viewNormal, roughness);
    output.viewPosition = float4(viewPos, viewPos.z); // w에 depth 저장
    
    return output;
}
