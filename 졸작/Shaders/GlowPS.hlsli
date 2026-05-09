#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(EffectVertexOut input) : SV_TARGET
{
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (textureIndex != 0xFFFFFFFF)
    {
        texColor = bindlessTextures[textureIndex].Sample(linearSampler, input.uv);
    }

    float3 color = texColor.rgb * effectColor.rgb;
    float finalAlpha = texColor.a * input.alpha * effectColor.a;

    return float4(color, finalAlpha);
}
