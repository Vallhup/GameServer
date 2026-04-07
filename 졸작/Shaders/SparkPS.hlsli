#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(EffectVertexOut input) : SV_TARGET
{
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (textureIndex != 0xFFFFFFFF)
    {
        texColor = bindlessTextures[textureIndex].Sample(linearSampler, input.uv);
    }

    float flameIntensity = texColor.r;

    // 중앙 글로우 (양방향 균일 — 길쭉함 없음)
    float2 center = input.uv - float2(0.5f, 0.5f);
    float centerGlow = 1.0f - length(center) * 2.0f;
    centerGlow = saturate(centerGlow);
    centerGlow = pow(centerGlow, 1.5f);

    float finalAlpha = input.alpha * flameIntensity * centerGlow;

    // 3단계 그라데이션: 흰색(중심) → 노란색(중간) → 주황색(외곽)
    float3 white = float3(1.0f, 1.0f, 0.95f);
    float3 yellow = float3(1.0f, 0.85f, 0.25f);
    float3 orange = effectColor.rgb * float3(1.0f, 0.2f, 0.1f);

    float t = centerGlow * flameIntensity;
    float yellowBlend = smoothstep(0.0f, 0.99f, t);
    float whiteBlend = pow(saturate(t), 7.0f);

    float3 baseColor = lerp(orange, yellow, yellowBlend);
    baseColor = lerp(baseColor, white, whiteBlend);
    float3 color = baseColor * flameIntensity * (1.0f + centerGlow * 0.5f);

    return float4(color, finalAlpha * effectColor.a);
}
