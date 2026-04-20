#include "ShaderResources.hlsli"
#include "InOutFormats.hlsli"

float4 PSMain(EffectVertexOut input) : SV_TARGET
{
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (textureIndex != 0xFFFFFFFF)
    {
        texColor = bindlessTextures[textureIndex].Sample(linearSampler, input.uv);
    }

    // 흑백 텍스처(Grayscale)이므로 R 채널의 값을 불꽃의 강도(Intensity)로 사용
    float flameIntensity = texColor.r;

    // 중앙은 밝게, 외곽은 자연스럽게 페이드 아웃 되도록 (X축 기반)
    float centerGlow = 1.0f - abs(input.uv.x - 0.5f) * 4.0f;

    // Y 위치에 따라 가로 폭 조절 (위/아래 얇게, 중간 넓게)
    float widthScale = smoothstep(0.0f, 0.3f, input.uv.y) * smoothstep(1.0f, 0.3f, input.uv.y);
    centerGlow = pow(max(centerGlow, 0.0), lerp(7.0f, 1.5f, widthScale));

    // 최종 알파 계산: 파티클 고유의 알파(수명 주기 페이드) * 텍스처 강도 * 보정값
    float finalAlpha = input.alpha * flameIntensity * centerGlow;

    // 3단계 그라데이션: 흰색(중심) → 노란색(중간) → 주황색(외곽)
    float3 white = float3(1.0f, 1.0f, 0.95f);
    float3 yellow = float3(1.0f, 0.85f, 0.25f);
    float3 orange = effectColor.rgb * float3(1.0f, 0.2f, 0.1f);  // 더 진한 주황

    float t = centerGlow * flameIntensity;
    float yellowBlend = smoothstep(0.0f, 0.99f, t);   // 주황 → 노랑
    float whiteBlend = pow(saturate(t), 7.0f);       // 노랑 → 흰색 (아주 중심부만)

    float3 baseColor = lerp(orange, yellow, yellowBlend);
    baseColor = lerp(baseColor, white, whiteBlend);
    float3 color = baseColor * flameIntensity * (1.0f + centerGlow * 0.5f);

    return float4(color, finalAlpha * effectColor.a);
}