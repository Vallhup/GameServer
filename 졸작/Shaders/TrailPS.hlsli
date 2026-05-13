#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(EffectVertexOut input) : SV_TARGET
{
    float lengthFade = saturate(1.0f - input.uv.y / 0.3f);    
    float timeFade   = input.uv.x * input.uv.x;               

    float finalAlpha = input.alpha * lengthFade * timeFade;
    float3 color     = effectColor.rgb;

    return float4(color, finalAlpha * effectColor.a);
}
