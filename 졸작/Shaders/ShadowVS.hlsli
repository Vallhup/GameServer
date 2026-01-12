#include "Skinning.hlsli"
#include "InOutFormats.hlsli"

SHADOW_VS_OUT VSMain(SHADOW_VS_IN input)
{
    SHADOW_VS_OUT output;
    
    float3 modifiedPos = input.pos;
    
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;
    bool hasAnimation = (totalWeight > 0.001f);
    
    if (hasAnimation)
    {
        SkinningPosition(modifiedPos, input.weights, input.indices);
    }
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), world);
    
    float4 lightViewPos = mul(worldPos, lightView);
    output.pos = mul(lightViewPos, lightProjection);
    output.uv = input.uv;
    output.materialIndex = materialIndex;
    
    return output;
}