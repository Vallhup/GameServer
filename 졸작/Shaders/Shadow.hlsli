#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "ShaderResources.hlsli"

float CalculateShadow(float3 worldPos)
{
    float4 lightSpacePos = mul(float4(worldPos, 1.0), lightView);
    lightSpacePos = mul(lightSpacePos, lightProjection);
    
    lightSpacePos.xyz /= lightSpacePos.w;
    
    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
        shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0;
    
    float currentDepth = lightSpacePos.z;
    float shadowMapDepth = shadowMap.Sample(linearSampler, shadowUV).r;
    
    float bias = 0.0001f;
    float shadow = 0.0f;
    float2 texelSize = 1.0 / 2048.0;
    
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float shadowMapDepth = shadowMap.Sample(linearSampler, shadowUV + offset).r;
            
            if ((currentDepth - bias) > shadowMapDepth)
                shadow += 1.0f;
        }
    }

    shadow /= 25.0;
    return lerp(1.0, 0.7, shadow);
}

#endif