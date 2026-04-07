#include "Skinning.hlsli"
#include "InOutFormats.hlsli"
#include "VertexAnimation.hlsli"

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
    
    if (useVertexAnim && input.color.r > 0.1f)
    {
        VertexAnimation(modifiedPos, worldMatrix, input.color, time);
    }
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), worldMatrix);
    
    output.pos = mul(worldPos, lightVP[cascadeIndex]);
    output.uv = input.uv;
    output.materialIndex = materialIndex;
    
    return output;
}