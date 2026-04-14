#include "InOutFormats.hlsli"
#include "VolumetricFog.hlsli"

float4 PSMain(LIGHTING_PS_IN input) : SV_Target
{
    float depth = depthBuffer.Sample(pointSampler, input.uv).r;
    
    if (depth >= 1.0f)
        return float4(0, 0, 0, 1);
    
    float2 ndc = float2(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0);
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 wp = mul(clipPos, invViewProj);
    float3 worldPos = wp.xyz / wp.w;
    
    float3 rayOrigin = cameraPosition;
    float3 rayDir = normalize(worldPos - cameraPosition);
    float sceneDepth = length(worldPos - cameraPosition);
    
    return RayMarchingVolumetricFog(rayOrigin, rayDir, sceneDepth, input.uv);
}