#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(SKYBOX_PS_IN input) : SV_Target
{
    float3 direction = normalize(input.localPos);
    float3 color = bindlessCubeMaps[NonUniformResourceIndex(0)].Sample(linearSampler, direction).rgb;
    return float4(color, 1.0f);
}