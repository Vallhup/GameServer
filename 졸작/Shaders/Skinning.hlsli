#ifndef SKINNING_HLSLI
#define SKINNING_HLSLI

#include "ShaderResources.hlsli"

void SkinningFull(inout float3 pos, inout float3 normal, inout float3 tangent, inout float4 weight, inout float4 indices)
{
    float3 skinnedPos = float3(0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);
    float3 skinnedTangent = float3(0, 0, 0);

    for (int i = 0; i < 4; ++i)
    {
        if (weight[i] == 0.f)
            continue;

        int boneIdx = (int) indices[i];
        matrix matBone = finalBoneTransforms[boneIdx];

        skinnedPos += (mul(float4(pos, 1.f), matBone) * weight[i]).xyz;
        skinnedNormal += (mul(float4(normal, 0.f), matBone) * weight[i]).xyz;
        skinnedTangent += (mul(float4(tangent, 0.f), matBone) * weight[i]).xyz;
    }

    pos = skinnedPos;
    normal = normalize(skinnedNormal);
    tangent = normalize(skinnedTangent);
}

void SkinningPosition(inout float3 pos, inout float4 weight, inout float4 indices)
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

#endif