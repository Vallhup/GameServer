#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(SKYBOX_PS_IN input) : SV_Target
{
    float3 direction = normalize(input.localPos);
    float3 color = bindlessCubeMaps[NonUniformResourceIndex(0)].Sample(linearSampler, direction).rgb;
    
    color *= skyExposure;
    color *= skyTintColor;

    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));       // Rec. 709 (sRGB)
    color = lerp(luminance.xxx, color, skySaturation);
    
    return float4(color, 1.0f);
}