// ============================================================================
// SSAO View Space G-Buffer - Pixel Shader
// World Space G-Buffer를 읽어서 View Space로 변환하여 저장
// ============================================================================

#include "ShaderResources.hlsli"

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

PS_OUT PSMain(PS_IN input)
{
    PS_OUT output;
    
    // === 1. World Space G-Buffer 샘플링 ===
    float4 worldNormalRoughness = gBufferRT1.Sample(linearSampler, input.uv);
    float4 worldPosAO = gBufferRT2.Sample(linearSampler, input.uv);
    
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
