#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(EffectVertexOut input) : SV_TARGET
{
    // UV.x에 따른 그라데이션 (꼬리 → 머리)
    float gradientFactor = input.uv.x;

    // 중앙이 밝고 가장자리가 어두운 효과 (UV.y 기반)
    float centerGlow = 1.0f - abs(input.uv.y - 0.5f) * 2.0f;
    centerGlow = pow(centerGlow, 0.5f);  // 부드러운 페이드

    // 최종 알파 계산
    float finalAlpha = input.alpha * centerGlow * gradientFactor;

    // 색상에 글로우 효과 추가
    float3 color = effectColor.rgb * (1.0f + centerGlow * 0.5f);

    return float4(color, finalAlpha * effectColor.a);
}
