#ifndef SSAOBLURPS_HLSLI
#define SSAOBLURPS_HLSLI

#include "InOutFormats.hlsli"
#include "ShaderResources.hlsli"

float4 PSMain(SSAO_PS_IN input) : SV_Target
{
    float2 texelSize = 1.0 / float2(noiseScale.x * 4.0, noiseScale.y * 4.0);

    float result = 0.0;

    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += ssaoResult.Sample(pointSampler, input.uv + offset).r;
        }
    }

    result /= 25.0;
    return float4(result, result, result, 1.0);
}

#endif