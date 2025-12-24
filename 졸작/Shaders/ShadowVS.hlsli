#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

void Skinning(inout float3 pos, inout float4 weight, inout float4 indices)
{
    float3 skinnedPos = float3(0, 0, 0);

    for (int i = 0; i < 4; ++i)
    {
        if (weight[i] == 0.f)
            continue;

        int boneIdx = (int) indices[i];
        matrix matBone = finalBoneTransforms[boneIdx];
        skinnedPos += (mul(float4(pos, 1.f), matBone) * weight[i]).xyz;
    }

    pos = skinnedPos;
}

SHADOW_VS_OUT VSMain(SHADOW_VS_IN input)
{
    SHADOW_VS_OUT output;
    
    float3 modifiedPos = input.pos;
    
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;
    bool hasAnimation = (totalWeight > 0.001f);
    
    if (hasAnimation)
    {
        Skinning(modifiedPos, input.weights, input.indices);
    }
    
    float4 worldPos = mul(float4(modifiedPos, 1.0f), world);
    
    float4 lightViewPos = mul(worldPos, lightView);
    output.pos = mul(lightViewPos, lightProjection);
    
    return output;
}