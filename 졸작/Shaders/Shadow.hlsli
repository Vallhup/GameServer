#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "ShaderResources.hlsli"
#include "Constants.hlsli"

float SampleCascadeShadow(float3 worldPos, float3 N, int cascade)
{
    float3 biasedPos = worldPos + N * cascadeNormalOffset[cascade];
    float4 lightSpacePos = mul(float4(biasedPos, 1.0), lightVP[cascade]);
    lightSpacePos.xyz /= lightSpacePos.w;

    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0;

    float currentDepth = lightSpacePos.z;
    float refDepth = currentDepth - cascadeBias[cascade];

    float2 texelSize = 1.0 / 4096.0;

    float lit = 0.0f;

    [unroll]
    for (int x = -2; x <= 2; ++x)
    {
        [unroll]
        for (int y = -2; y <= 2; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            lit += shadowMapArray.SampleCmpLevelZero(
                shadowCmpSampler,
                float3(shadowUV + offset, cascade),
                refDepth);
        }
    }

    return lit / 25.0;
}

// 실내(성당) overhead 동적 그림자: lightVP[0](위→아래 ortho)만 사용, 슬라이스 0 샘플.
float SampleOverheadShadow(float3 worldPos, float3 N)
{
    float3 biasedPos = worldPos + N * 0.05;
    float4 lightSpacePos = mul(float4(biasedPos, 1.0), lightVP[0]);
    lightSpacePos.xyz /= lightSpacePos.w;

    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0;

    float refDepth = lightSpacePos.z - 0.0015;
    float2 texelSize = 1.0 / 4096.0;

    float lit = 0.0f;
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            lit += shadowMapArray.SampleCmpLevelZero(
                shadowCmpSampler, float3(shadowUV + offset, 0), refDepth);
        }
    }
    return lit / 9.0;
}

float CalculateShadow(float3 worldPos, float3 N, float viewDepth)
{
    float splits[3] = { cascadeSplit.x, cascadeSplit.y, cascadeSplit.z };

    int cascade = 2;
    if (viewDepth < splits[0])
        cascade = 0;
    else if (viewDepth < splits[1])
        cascade = 1;

    float shadow = SampleCascadeShadow(worldPos, N, cascade);

    if (cascade < 2)
    {
        float splitEnd = splits[cascade];
        float blendStart = splitEnd * (1.0 - CASCADE_BLEND_RANGE);

        if (viewDepth > blendStart)
        {
            float blendFactor = (viewDepth - blendStart) / (splitEnd - blendStart);
            float nextShadow = SampleCascadeShadow(worldPos, N, cascade + 1);
            shadow = lerp(shadow, nextShadow, blendFactor);
        }
    }

    return shadow;
}

#endif