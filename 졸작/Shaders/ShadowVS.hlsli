#include "Skinning.hlsli"
#include "InOutFormats.hlsli"

SHADOW_VS_OUT VSMain(SHADOW_VS_IN input, uint instanceID : SV_InstanceID)
{
    SHADOW_VS_OUT output;
    
    float3 modifiedPos = input.pos;
    
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;
    bool hasAnimation = (totalWeight > 0.001f);
    
    if (useInstancing)
        hasAnimation = false;
    
    if (hasAnimation)
    {
        SkinningPosition(modifiedPos, input.weights, input.indices);
    }
    
    matrix worldMatrix;
    
    if (useInstancing)
        worldMatrix = instanceTransforms[instanceID];
    else
        worldMatrix = world;
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), worldMatrix);
    
    float4 lightViewPos = mul(worldPos, lightView);
    output.pos = mul(lightViewPos, lightProjection);
    output.uv = input.uv;
    output.materialIndex = materialIndex;
    
    return output;
}