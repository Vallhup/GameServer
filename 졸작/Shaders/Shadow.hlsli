#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "ShaderResources.hlsli"
#include "Constants.hlsli"

int SelectCascade(float viewDepth)
{
    if (viewDepth < cascadeSplit.x)
        return 0;
    if (viewDepth < cascadeSplit.y)
        return 1;
    if (viewDepth < cascadeSplit.z)
        return 2;
    return 3;
}

float CalculateShadow(float3 worldPos, float viewDepth)
{
    int cascade = SelectCascade(viewDepth);
    
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightVP[cascade]);
    
    lightSpacePos.xyz /= lightSpacePos.w;
    
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0;
    
    float currentDepth = lightSpacePos.z;
    
    float shadow = 0.0f;
    float2 texelSize = 1.0 / 4096.0;
    
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float shadowMapDepth = shadowMapArray.Sample(linearSampler, float3(shadowUV + offset, cascade)).r;
            
            if ((currentDepth - cascadeBias[cascade]) > shadowMapDepth)
                shadow += 1.0f;
        }
    }

    shadow /= 25.0;
    return lerp(1.0, 0.6, shadow);
}

#endif