#include "Skinning.hlsli"
#include "InOutFormats.hlsli"

FORWARD_VS_OUT VSMain(FORWARD_VS_IN input, uint instanceID : SV_InstanceID)
{
    FORWARD_VS_OUT output;
    
    float3 modifiedPos = input.pos;
    float3 modifiedNormal = input.normal;
    float3 modifiedTangent = input.tangent;
    
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;
    bool hasAnimation = (totalWeight > 0.001f);
    
    if (useInstancing)
        hasAnimation = false;
    
    if (hasAnimation)
    {
        SkinningFull(modifiedPos, modifiedNormal, modifiedTangent, input.weights, input.indices);
    }
    
    matrix worldMatrix;
    
    if (useInstancing)
        worldMatrix = instanceTransforms[instanceID];
    else
        worldMatrix = world;
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, projection);
    
    output.color = input.color;
    output.uv = input.uv;
    output.normal = normalize(mul(float4(modifiedNormal, 0.0f), worldMatrix).xyz);
    output.tangent = normalize(mul(float4(modifiedTangent, 0.0f), worldMatrix).xyz);
    output.weights = input.weights;
    output.indices = input.indices;
    output.materialIndex = materialIndex;
    output.worldPos = worldPos.xyz;
    output.clipDist = dot(worldPos, waterClipPlane);
    
    return output;
}