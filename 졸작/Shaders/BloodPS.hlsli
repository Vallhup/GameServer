#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(EffectVertexOut input) : SV_TARGET
{
    float4 texColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (textureIndex != 0xFFFFFFFF)
    {
        texColor = bindlessTextures[textureIndex].Sample(linearSampler, input.uv);
    }
    
    float mask = dot(texColor.rgb, float3(0.299f, 0.587f, 0.114f));
    
    float3 rgb = texColor.rgb * effectColor.rgb;

    float finalAlpha = mask * input.alpha * effectColor.a;

    return float4(rgb, finalAlpha);
}
