#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

EffectVertexOut VSMain(EffectVertexIn input)
{
    EffectVertexOut output;

    float4 worldPos = float4(input.position, 1.0f);
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    output.uv = input.uv;
    output.alpha = input.alpha;

    return output;
}